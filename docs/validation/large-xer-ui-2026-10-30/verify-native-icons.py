from pathlib import Path
import ctypes, json, struct
k = ctypes.WinDLL('kernel32', use_last_error=True)
k.LoadLibraryExW.argtypes=[ctypes.c_wchar_p,ctypes.c_void_p,ctypes.c_uint32]; k.LoadLibraryExW.restype=ctypes.c_void_p
k.FindResourceW.argtypes=[ctypes.c_void_p,ctypes.c_void_p,ctypes.c_void_p]; k.FindResourceW.restype=ctypes.c_void_p
k.SizeofResource.argtypes=[ctypes.c_void_p,ctypes.c_void_p]; k.SizeofResource.restype=ctypes.c_uint32
k.LoadResource.argtypes=[ctypes.c_void_p,ctypes.c_void_p]; k.LoadResource.restype=ctypes.c_void_p
k.LockResource.argtypes=[ctypes.c_void_p]; k.LockResource.restype=ctypes.c_void_p
k.FreeLibrary.argtypes=[ctypes.c_void_p]
callback=ctypes.WINFUNCTYPE(ctypes.c_bool,ctypes.c_void_p,ctypes.c_void_p,ctypes.c_void_p,ctypes.c_ssize_t)
k.EnumResourceNamesW.argtypes=[ctypes.c_void_p,ctypes.c_void_p,callback,ctypes.c_ssize_t]
results=[]
for path in (Path('build/JOBS.exe'),Path('dist/JOBS-0.2.0-Setup-win64-ab251153.exe')):
    module=k.LoadLibraryExW(str(path.resolve()),None,2)
    assert module, ctypes.get_last_error()
    groups=[]
    @callback
    def collect(handle,kind,name,unused):
        resource=k.FindResourceW(handle,name,kind)
        size=k.SizeofResource(handle,resource)
        data=ctypes.string_at(k.LockResource(k.LoadResource(handle,resource)),size)
        count=struct.unpack_from('<H',data,4)[0]
        groups.append({'id':name if name<65536 else ctypes.wstring_at(name),'images':count})
        return True
    try: assert k.EnumResourceNamesW(module,14,collect,0)
    finally: k.FreeLibrary(module)
    assert any(group['images']==10 for group in groups),groups
    results.append({'path':str(path),'icon_groups':groups,'status':'passed'})
Path('build/native-icon-resources.json').write_text(json.dumps(results,indent=2)+'\n')
print(json.dumps(results,indent=2))
