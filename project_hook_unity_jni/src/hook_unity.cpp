// hook_unity.cpp
// Created by sisong on 2019-08-15.

#include "hook_unity.h"
#include <string.h>
#include <stdio.h>
#include <stdlib.h> //exit
#include <sys/stat.h>
#include <fcntl.h>
#include "../bhook/bytehook/src/main/cpp/include/bytehook.h"
#include <android/log.h>
#include <dlfcn.h> // dlopen
#ifdef __cplusplus
extern "C" {
#endif
    static const char* kLibMain="libmain.so";
    static const char* kLibUnity="libunity.so";
    static const int   kLibUnityLen=11;//strlen(kLibUnity);
    //for il2cpp
    static const char* kLibIL2cpp="libil2cpp.so";
    
    #define _IsDebug 0
    #define _LogTag "HotUnity"
    #define LOG_INFO(fmt,args...)       __android_log_print(ANDROID_LOG_INFO,_LogTag,fmt, ##args)
    #define LOG_ERROR(fmt,args...)      __android_log_print(ANDROID_LOG_ERROR,_LogTag,fmt, ##args)
    #define LOG_DEBUG(fmt,args...)      do { if (_IsDebug) LOG_INFO(fmt, ##args); } while(0)

    static const int  kMaxPathLen=512-1;
    static const char kDirTag='/';
    
    static bool g_isMapPath =false;
    static char g_baseApkPath[kMaxPathLen+1]={0};
    static  int g_baseApkPathLen=0;
    static char g_baseSoDir[kMaxPathLen+1]={0};
    static  int g_baseSoDirLen=0;
    static char g_hotApkPath[kMaxPathLen+1]={0};
    static  int g_hotApkPathLen=0;
    static char g_hotSoDir[kMaxPathLen+1]={0};
    static  int g_hotSoDirLen=0;
    
    
    static const char*  getFileName(const char* filePath){
        int pathLen=(int)strlen(filePath);
        int i=pathLen-1;
        for (;i>=0;--i) {
            if (filePath[i]==kDirTag) break;
        }
        return filePath+(i+1);
    }
    
    static bool pathIsExists(const char* path){
        struct stat path_stat;
        memset(&path_stat,0,sizeof(path_stat));
        int ret=stat(path,&path_stat);
        return (ret==0)&&( (path_stat.st_mode&(S_IFREG|S_IFDIR))!=0 );
    }
    
    static bool appendPath(char* dstPath,int dstPathSize,
                           const char* dir,int dirLen,const char*subPath,int subPathLen){
        if (dirLen+1+subPathLen+1>dstPathSize){
            LOG_ERROR("appendPath() error len  %s + %s",dir,subPath);
            return false;
        }
        memcpy(dstPath,dir,dirLen);
        dstPath+=dirLen;
        int dirTagCount= (((dirLen>0)&&(dir[dirLen-1]==kDirTag))?1:0)
                        +(((subPathLen>0)&&(subPath[0]==kDirTag))?1:0);
        switch (dirTagCount) {
            case 2:{ ++subPath; --subPathLen; } break;
            case 0:{ if ((dirLen>0)&&(subPathLen>0)) { dstPath[0]=kDirTag; ++dstPath; } } break;
        }
        memcpy(dstPath,subPath,subPathLen);
        dstPath[subPathLen]='\0';
        return true;
    }
    
    static const char* map_path(const char* src,int srcLen,
                                const char* dst,int dstLen,
                                const char* path,int pathLen,char* newPath,const int newPathSize,
                                bool needPathIsExists,bool* isCanMap=NULL,int clipLen=-1){
        const char* const errValue=NULL;
        if (isCanMap) *isCanMap=false;
        if (!g_isMapPath) return path;
        if ((pathLen<srcLen)||(0!=memcmp(path,src,srcLen))) return path;
        if ((path[srcLen]!='\0')&&(path[srcLen]!=kDirTag)) return path;
        
        if (isCanMap) *isCanMap=true;
        if (clipLen<0) clipLen=srcLen;
        if (!appendPath(newPath,newPathSize,dst,dstLen,path+clipLen,pathLen-clipLen)) return errValue;
        
        if ((!needPathIsExists)||pathIsExists(newPath)){
            LOG_DEBUG("map_path() to %s",newPath);
            return newPath;
        }else{
            LOG_DEBUG("map_path() not found %s",newPath);
            return path;
        }
    }
    
    static const char* map_path(const char* path,char* newPath,const int newPathSize,bool* isSoDirCanMap){
        const int pathLen=strlen(path);
        path=map_path(g_baseApkPath,g_baseApkPathLen,g_hotApkPath,g_hotApkPathLen,
                      path,pathLen,newPath,newPathSize,false,NULL,-1);
        if ((path==NULL)||(path==newPath)) return path;
        path=map_path(g_baseSoDir,g_baseSoDirLen,g_hotSoDir,g_hotSoDirLen,
                      path,pathLen,newPath,newPathSize,true,isSoDirCanMap,-1);
        if ((path==NULL)||(path==newPath)) return path;
        return map_path(kLibUnity,kLibUnityLen,g_hotSoDir,g_hotSoDirLen,
                        path,pathLen,newPath,newPathSize,true,NULL,0);
    }

    #define MAP_PATH(opath,errValue)    \
        char _newPath[kMaxPathLen+1];   \
        bool isSoDirCanMap=false;       \
        opath=map_path(opath,_newPath,sizeof(_newPath),&isSoDirCanMap); \
        if (opath==NULL) return errValue;
    
    //stat
    static int new_stat(const char* path,struct stat* file_stat){
        BYTEHOOK_STACK_SCOPE();
        const int errValue=-1;
        LOG_DEBUG("new_stat() %s",path);
        MAP_PATH(path,errValue);
        return ::stat(path,file_stat);
    }
    
    //fopen
    static FILE* new_fopen(const char* path,const char* mode){
        BYTEHOOK_STACK_SCOPE();
        FILE* const errValue=NULL;
        LOG_DEBUG("new_fopen() %s %s",mode,path);
        MAP_PATH(path,errValue);
        return ::fopen(path,mode);
    }
    
    //open
    static int new_open(const char *path, int flags, ...){
        BYTEHOOK_STACK_SCOPE();
        const int errValue=-1;
        LOG_DEBUG("new_open() %d %s",flags,path);
        MAP_PATH(path,errValue);
        
        va_list args;
        va_start(args,flags);
        int result=::open(path,flags,args);
        va_end(args);
        return result;
    }
    
#if (_IsDebug>=2)
    static void _DEBUG_log_libmaps(){
        const char* path="/proc/self/maps";
        FILE* fp=fopen(path,"r");
        char line[(kMaxPathLen+1)*2]={0};
        while(fgets(line,sizeof(line)-1, fp)){
            LOG_DEBUG(" lib maps : %s\n",line);
        }
        fclose(fp);
    }
#else
    #define _DEBUG_log_libmaps()
#endif
    
    //dlopen
    static bool hook_lib(const char* libPath);
    static void* my_new_dlopen(const char* path,int flags,bool isMustLoad,bool isMustHookOk){
        void* const errValue=NULL;
        LOG_DEBUG("new_dlopen() %d %s",flags,path);
        MAP_PATH(path,errValue);
        if ((!isMustLoad)&&(!pathIsExists(path)))
            return errValue;
        
        void* result=::dlopen(path,flags);
        LOG_INFO("dlopen() result 0x%08x %s",(unsigned int)(size_t)result,path);
        _DEBUG_log_libmaps();
        
        if ((result!=errValue)&&isSoDirCanMap){
            bool ret=hook_lib(path);
            if ((!ret)&&isMustHookOk)
                return errValue;
        }
        return result;
    }
    
    static void* new_dlopen(const char* path,int flags){
        BYTEHOOK_STACK_SCOPE();
        return my_new_dlopen(path,flags,true,false);
    }

#define BYTE_HOOK_DEF(fn)                                                                                         \
  static bytehook_stub_t fn##_stub = NULL;                                                                   \
  static void fn##_hooked_callback(bytehook_stub_t task_stub, int status_code, const char *caller_path_name, \
                                   const char *sym_name, void *new_func, void *prev_func, void *arg) {       \
  }

BYTE_HOOK_DEF(stat);
BYTE_HOOK_DEF(fopen);
BYTE_HOOK_DEF(open);
BYTE_HOOK_DEF(dlopen);

    static bool hook_lib(const char* libPath){
        LOG_INFO("hook_lib() to hook %s",libPath);
        libPath=getFileName(libPath);
#if (_IsDebug>=2)
        bytehook_set_debug(true);
#endif
        const bool errValue=false;
        stat_stub=bytehook_hook_single(libPath,NULL,"stat",(void*)new_stat,stat_hooked_callback,NULL);
        if (NULL==stat_stub){ LOG_ERROR("hook_lib() failed to hook stat"); return errValue; }
        fopen_stub=bytehook_hook_single(libPath,NULL,"fopen",(void*)new_fopen,fopen_hooked_callback,NULL);
        if (NULL==fopen_stub){ LOG_ERROR("hook_lib() failed to hook fopen"); return errValue; }
        open_stub=bytehook_hook_single(libPath,NULL,"open",(void*)new_open,open_hooked_callback,NULL);
        if (NULL==open_stub){ LOG_ERROR("hook_lib() failed to hook open"); return errValue; }
        dlopen_stub=bytehook_hook_single(libPath,NULL,"dlopen",(void*)new_dlopen,dlopen_hooked_callback,NULL);
        if (NULL==dlopen_stub){ LOG_ERROR("hook_lib() failed to hook dlopen"); return errValue; }
        return true;
    }
    
    
    static bool loadUnityLib(const char* libName,bool isMustLoad,bool isMustHookOk){
        char libPath[kMaxPathLen+1];
        if (!appendPath(libPath,sizeof(libPath),
                        g_baseSoDir,g_baseSoDirLen,libName,(int)strlen(libName))) return false;
        
        const int flags= RTLD_NOW;
        void* result=my_new_dlopen(libPath,flags,isMustLoad,isMustHookOk);
        return (result!=NULL)||(!isMustLoad);
    }
    
    static bool loadUnityLibs(){
        bytehook_init(0,false);
        if (!hook_lib(kLibMain)) return false; // loaded in java, only hook
        if (!hook_lib(kLibUnity)) return false; // loaded in java, only hook
        if (!loadUnityLib(kLibIL2cpp,false,false)) return false; // pre-load for il2cpp
        //test found : not need pre-load libs for mono
        return true;
    }
    
    
    #define _ERR_RETURN() { g_isMapPath=false; exit(-1); return; }
    
    #define _COPY_PATH(dst,src) {   \
        dst##Len=strlen(src);       \
        if (dst##Len>kMaxPathLen)   \
            { LOG_ERROR("hook_unity_doHook() strlen("#src") %d",dst##Len); _ERR_RETURN(); } \
        memcpy(dst,src,dst##Len+1); }
    
    void hook_unity_doHook(const char* baseApkPath,const char* baseSoDir,
                           const char* hotApkPath,const char* hotSoDir){
        LOG_INFO("hook_unity_doHook() from: %s  %s",baseApkPath,baseSoDir);
        LOG_INFO("hook_unity_doHook() to: %s  %s",hotApkPath,hotSoDir);
        
        _COPY_PATH(g_baseApkPath,baseApkPath);
        _COPY_PATH(g_baseSoDir,baseSoDir);
        _COPY_PATH(g_hotApkPath,hotApkPath);
        _COPY_PATH(g_hotSoDir,hotSoDir);
        
        g_isMapPath=true;
        if (!loadUnityLibs()) _ERR_RETURN();
    }

#ifdef __cplusplus
}
#endif

