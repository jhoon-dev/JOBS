#include "ui/MainWindow.h"
#include "persistence/ProjectXml.h"
#include "persistence/JsonProjectStore.h"
#include <QApplication>
#include <QAction>
#include <QDialog>
#include <QDialogButtonBox>
#include <QDir>
#include <QFile>
#include <QFileDialog>
#include <QLineEdit>
#include <QMessageBox>
#include <QPlainTextEdit>
#include <QPushButton>
#include <QTemporaryDir>
#include <QTest>
#include <QTimer>
#include <functional>
#include <iostream>

namespace {
void check(bool b,const char* message){if(!b)throw std::runtime_error(message);}
void callback(bool b,const char* message){if(!b){std::cerr<<message<<'\n';std::_Exit(2);}}
void action(QWidget& w,const char* name){auto* a=w.findChild<QAction*>(name);check(a&&a->isEnabled(),"action missing/disabled");a->trigger();}
// Poll actual modal widgets; file-dialog completion and preview creation are separate turns.
struct Drive {
    QTimer timer;QString input,destination;bool accept=true,errorExpected=false,seenPreview=false,seenError=false,captured=false;
    QMessageBox::StandardButton unsaved=QMessageBox::Discard;
    std::function<void()> beforeAccept;
    Drive(QString in,QString out):input(std::move(in)),destination(std::move(out)){
        timer.setInterval(20);QObject::connect(&timer,&QTimer::timeout,[this]{
            auto* modal=QApplication::activeModalWidget();if(!modal)return;
            if(auto* file=qobject_cast<QFileDialog*>(modal)){
                auto* line=file->findChild<QLineEdit*>("fileNameEdit");callback(line,"file input missing");line->setText(input);QMetaObject::invokeMethod(file,"accept");return;
            }
            if(modal->objectName()=="projectXmlPreview"){
                if(seenPreview)return;
                seenPreview=true;
                auto* detail=modal->findChild<QPlainTextEdit*>("xmlDiagnostics");callback(detail&&detail->isReadOnly()&&detail->toPlainText().contains("미검증")&&detail->toPlainText().contains("동반"),"preview lacks source/loss/verdict");
                auto* path=modal->findChild<QLineEdit*>("xmlDestination");callback(path,"destination missing");path->setText(destination);
                modal->grab().save("project-xml-preview-native.png");captured=true;
                modal->grab().save(input.isEmpty()?"project-xml-export-native.png":"project-xml-import-native.png");
                if(beforeAccept)beforeAccept();
                auto* buttons=modal->findChild<QDialogButtonBox*>();callback(buttons,"preview buttons missing");buttons->button(accept?QDialogButtonBox::Ok:QDialogButtonBox::Cancel)->click();return;
            }
            if(auto* box=qobject_cast<QMessageBox*>(modal)){
                // macOS ignores QMessageBox titles. Identify the unsaved-changes
                // contract by its standard buttons, not by window decoration.
                const auto buttons=box->standardButtons();
                if(buttons.testFlag(QMessageBox::Save)&&buttons.testFlag(QMessageBox::Discard)&&buttons.testFlag(QMessageBox::Cancel)){box->button(unsaved)->click();return;}
                if(!errorExpected)std::cerr<<"Unexpected dialog: "<<box->text().toUtf8().constData()<<'\n';
                callback(errorExpected,"unexpected error dialog");seenError=true;box->accept();
            }
        });timer.start();
    }
};
}
int main(int argc,char** argv){QApplication app(argc,argv);app.setQuitOnLastWindowClosed(false);QApplication::setAttribute(Qt::AA_DontUseNativeDialogs);
QTimer watchdog;watchdog.setSingleShot(true);QObject::connect(&watchdog,&QTimer::timeout,[]{std::cerr<<"XML UI timeout\n";std::_Exit(2);});watchdog.start(45000);
try{
    QTemporaryDir dir;check(dir.isValid(),"temporary folder");jobs::JsonProjectStore store;jobs::Document doc(store);jobs::MainWindow window(doc);window.show();QTest::qWait(30);
    check(!window.findChild<QAction*>("exportProjectXml")->isEnabled(),"empty export enabled");
    doc.create("기존 프로젝트","2026-10-26",{"보존 작업"});
    // Refresh through an ordinary public action; a separate preexisting document is what import will replace.
    jobs::MainWindow current(doc);current.show();QTest::qWait(30);const auto original=*doc.project();const auto generation=doc.sessionGeneration();
    {Drive drive(QString::fromUtf8(JOBS_XML_FIXTURE),dir.filePath("cancel"));drive.accept=false;action(current,"importProjectXml");check(drive.seenPreview&&*doc.project()==original&&doc.sessionGeneration()==generation&&!QFileInfo::exists(drive.destination),"preview cancel changed document/files");}
    {Drive drive(QString::fromUtf8(JOBS_XML_FIXTURE),dir.filePath("unsaved-cancel"));drive.unsaved=QMessageBox::Cancel;action(current,"importProjectXml");check(drive.seenPreview&&*doc.project()==original&&!QFileInfo::exists(drive.destination),"unsaved cancel published import");}
    const auto destination=dir.filePath("accepted");
    {Drive drive(QString::fromUtf8(JOBS_XML_FIXTURE),destination);action(current,"importProjectXml");check(drive.seenPreview&&drive.captured&&doc.project()->tasks.size()==6&&!doc.dirty()&&doc.sessionGeneration()>generation,"import not opened as new saved session");}
    check(QFileInfo::exists(QDir(destination).filePath("original.xml"))&&QFileInfo::exists(QDir(destination).filePath("conversion.json")),"import missing source report");
    const auto imported=*doc.project();const auto importPath=doc.path();
    {Drive drive({},dir.filePath("export-cancel"));drive.accept=false;action(current,"exportProjectXml");check(drive.seenPreview&&*doc.project()==imported&&!QFileInfo::exists(drive.destination),"export cancel changed state");}
    {Drive drive({},dir.filePath("exported"));action(current,"exportProjectXml");check(drive.seenPreview&&*doc.project()==imported&&!doc.dirty()&&!doc.canUndo()&&doc.path()==importPath&&QFileInfo::exists(QDir(drive.destination).filePath("source.jobs")),"export changed document or lacked source");}
    {Drive drive({},dir.filePath("stale"));drive.errorExpected=true;drive.beforeAccept=[&]{doc.renameTask(0,"이전 미리보기 이후 변경");};action(current,"exportProjectXml");check(drive.seenError&&!QFileInfo::exists(drive.destination)&&doc.project()->revision==imported.revision+1,"stale preview exported");doc.undo();}
    // Bad XML fails before the preview and preserves current state.
    const auto bad=dir.filePath("bad.xml");QFile badFile(bad);check(badFile.open(QIODevice::WriteOnly),"bad fixture write");badFile.write("<Project/>");badFile.close();const auto beforeBad=*doc.project();
    {Drive drive(bad,dir.filePath("bad"));drive.errorExpected=true;action(current,"importProjectXml");check(drive.seenError&&!drive.seenPreview&&*doc.project()==beforeBad,"invalid import changed state");}
    jobs::Document readonly(store);readonly.open(importPath,true);jobs::MainWindow reader(readonly);reader.show();QTest::qWait(20);const auto readSnapshot=*readonly.project();
    {Drive drive({},dir.filePath("readonly-export"));action(reader,"exportProjectXml");check(drive.seenPreview&&*readonly.project()==readSnapshot&&readonly.readOnly()&&!readonly.dirty()&&QFileInfo::exists(QDir(drive.destination).filePath("project.xml")),"readonly export failed");}
    // Use a clean project snapshot to check support for empty native projects too.
    jobs::Project empty;empty.documentId="empty";empty.name="빈 프로젝트";empty.startDate="2026-10-26";auto emptyXml=jobs::exportProjectXml(empty);check(jobs::importProjectXml(emptyXml.xml,"empty.xml").project.tasks.empty(),"empty project exchange");
    std::cout<<"Project XML UI preview/cancel/import/export/stale/readonly tests passed\n";
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}return 0;}
