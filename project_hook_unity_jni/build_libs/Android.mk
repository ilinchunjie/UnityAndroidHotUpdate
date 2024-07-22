LOCAL_PATH := $(call my-dir)
include $(CLEAR_VARS)

LOCAL_MODULE := hotunity

ADP_PATH   := $(LOCAL_PATH)/../../ApkDiffPatch
 
Lzma_Files := $(ADP_PATH)/lzma/C/LzmaDec.c \
              $(ADP_PATH)/lzma/C/Lzma2Dec.c
ifeq ($(TARGET_ARCH_ABI),arm64-v8a)
  Lzma_Files += $(ADP_PATH)/lzma/Asm/arm64/LzmaDecOpt.S
endif

ZLIB_PATH  := $(ADP_PATH)/zlib1.2.11
Zlib_Files := $(ZLIB_PATH)/crc32.c    \
              $(ZLIB_PATH)/deflate.c  \
              $(ZLIB_PATH)/inflate.c  \
              $(ZLIB_PATH)/zutil.c    \
              $(ZLIB_PATH)/adler32.c  \
              $(ZLIB_PATH)/trees.c    \
              $(ZLIB_PATH)/inftrees.c \
              $(ZLIB_PATH)/inffast.c

HDP_PATH  := $(ADP_PATH)/HDiffPatch
Hdp_Files := $(HDP_PATH)/file_for_patch.c \
             $(HDP_PATH)/libHDiffPatch/HPatch/patch.c \
             $(HDP_PATH)/libParallel/parallel_import.cpp \
             $(HDP_PATH)/libParallel/parallel_channel.cpp

ADP_PATCH_PATH := $(ADP_PATH)/src/patch
Adp_Files := $(ADP_PATCH_PATH)/NewStream.cpp \
             $(ADP_PATCH_PATH)/OldStream.cpp \
             $(ADP_PATCH_PATH)/Patcher.cpp \
             $(ADP_PATCH_PATH)/ZipDiffData.cpp \
             $(ADP_PATCH_PATH)/Zipper.cpp

##XHOOK_PATH := $(LOCAL_PATH)/../xHook/libxhook/jni
##xHook_Files := \
##        $(XHOOK_PATH)/xh_core.c \
##        $(XHOOK_PATH)/xh_elf.c  \
##        $(XHOOK_PATH)/xh_log.c  \
##        $(XHOOK_PATH)/xh_util.c \
##        $(XHOOK_PATH)/xh_version.c

LOCAL_C_INCLUDES := \
        $(LOCAL_PATH)/../bhook/bytehook/src/main/cpp/third_party/bsd \
        $(LOCAL_PATH)/../bhook/bytehook/src/main/cpp/include \
        $(LOCAL_PATH)/../bhook/bytehook/src/main/cpp/third_party/lss

BHOOK_PATH := $(LOCAL_PATH)/../bhook/bytehook/src/main/cpp
BHook_Files := \
        $(BHOOK_PATH)/bh_cfi.c \
        $(BHOOK_PATH)/bh_core.c \
        $(BHOOK_PATH)/bh_dl_iterate.c \
        $(BHOOK_PATH)/bh_dl_monitor.c \
        $(BHOOK_PATH)/bh_dl.c \
        $(BHOOK_PATH)/bh_elf_manager.c \
        $(BHOOK_PATH)/bh_elf.c \
        $(BHOOK_PATH)/bh_hook_manager.c \
        $(BHOOK_PATH)/bh_hook.c \
        $(BHOOK_PATH)/bh_jni.c \
        $(BHOOK_PATH)/bh_linker_mutex.c \
        $(BHOOK_PATH)/bh_linker.c \
        $(BHOOK_PATH)/bh_log.c  \
        $(BHOOK_PATH)/bh_recorder.c  \
        $(BHOOK_PATH)/bh_sleb128.c  \
        $(BHOOK_PATH)/bh_task_manager.c \
        $(BHOOK_PATH)/bh_task.c \
        $(BHOOK_PATH)/bh_trampo_arm.c \
        $(BHOOK_PATH)/bh_trampo_arm64.c \
        $(BHOOK_PATH)/bh_trampo_x86_64.c \
        $(BHOOK_PATH)/bh_trampo_x86.c \
        $(BHOOK_PATH)/bh_trampo.c \
        $(BHOOK_PATH)/bh_util.c \
        $(BHOOK_PATH)/bytehook.c \
        $(BHOOK_PATH)/bytesig.c \

Src_Files := $(LOCAL_PATH)/../src/hook_unity.cpp \
             $(LOCAL_PATH)/../src/hook_unity_jni.cpp \
             $(LOCAL_PATH)/../src/virtual_apk_patch_jni.cpp \
             $(LOCAL_PATH)/../../VirtualApkPatch/patch/virtual_apk_patch.cpp

LOCAL_SRC_FILES  := $(Src_Files) $(BHook_Files) $(Lzma_Files) $(Zlib_Files) $(Hdp_Files) $(Adp_Files)

DEF_FLAGS :=  -DZ7_ST -D_IS_USED_MULTITHREAD=1 -D_IS_USED_PTHREAD=1 -D_IS_NEED_CACHE_OLD_BY_COVERS=0 \
             -D_IS_NEED_FIXED_ZLIB_VERSION=1 -D_IS_NEED_VIRTUAL_ZIP=1
ifeq ($(TARGET_ARCH_ABI),arm64-v8a)
  DEF_FLAGS += -DZ7_LZMA_DEC_OPT 
endif

LOCAL_LDLIBS := -llog -landroid
LOCAL_CFLAGS := -Os -DANDROID_NDK -DNDEBUG -D_LARGEFILE_SOURCE -DTARGET_ARCH_ABI=\"$(TARGET_ARCH_ABI)\" $(DEF_FLAGS)

include $(BUILD_SHARED_LIBRARY)