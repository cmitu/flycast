#include <jni.h>
#include <signal.h>
#include <string.h>
#include <stdio.h>
#include <stdarg.h>
#include <android/log.h>
#include <unistd.h>

#include <EGL/egl.h>
#include <GLES2/gl2.h>
#include <types.h>

#include "hw/maple/maple_cfg.h"
#include "profiler/profiler.h"
#include "rend/TexCache.h"
#include "hw/maple/maple_devs.h"
#include "hw/maple/maple_if.h"
#include "oslib/audiobackend_android.h"
#include "reios/reios.h"
#include "imgread/common.h"
#include "rend/gui.h"
#include "android_gamepad.h"

#define SETTINGS_ACCESSORS(jsetting, csetting, type)                                                                                                    \
JNIEXPORT type JNICALL Java_com_reicast_emulator_emu_JNIdc_get ## jsetting(JNIEnv *env, jobject obj)  __attribute__((visibility("default")));           \
JNIEXPORT void JNICALL Java_com_reicast_emulator_emu_JNIdc_set ## jsetting(JNIEnv *env, jobject obj, type v)  __attribute__((visibility("default")));   \
JNIEXPORT type JNICALL Java_com_reicast_emulator_emu_JNIdc_get ## jsetting(JNIEnv *env, jobject obj)                                                    \
{                                                                                                                                                       \
    return settings.csetting;                                                                                                                           \
}                                                                                                                                                       \
JNIEXPORT void JNICALL Java_com_reicast_emulator_emu_JNIdc_set ## jsetting(JNIEnv *env, jobject obj, type v)                                            \
{                                                                                                                                                       \
    settings.csetting = v;                                                                                                                              \
}

extern "C"
{
JNIEXPORT void JNICALL Java_com_reicast_emulator_emu_JNIdc_config(JNIEnv *env,jobject obj,jstring dirName)  __attribute__((visibility("default")));
JNIEXPORT void JNICALL Java_com_reicast_emulator_emu_JNIdc_init(JNIEnv *env,jobject obj,jstring fileName)  __attribute__((visibility("default")));
JNIEXPORT void JNICALL Java_com_reicast_emulator_emu_JNIdc_query(JNIEnv *env,jobject obj,jobject emu_thread, jobject emulator)  __attribute__((visibility("default")));
JNIEXPORT void JNICALL Java_com_reicast_emulator_emu_JNIdc_run(JNIEnv *env,jobject obj,jobject emu_thread)  __attribute__((visibility("default")));
JNIEXPORT void JNICALL Java_com_reicast_emulator_emu_JNIdc_pause(JNIEnv *env,jobject obj)  __attribute__((visibility("default")));
JNIEXPORT void JNICALL Java_com_reicast_emulator_emu_JNIdc_destroy(JNIEnv *env,jobject obj)  __attribute__((visibility("default")));

JNIEXPORT jint JNICALL Java_com_reicast_emulator_emu_JNIdc_send(JNIEnv *env,jobject obj,jint id, jint v)  __attribute__((visibility("default")));
JNIEXPORT jint JNICALL Java_com_reicast_emulator_emu_JNIdc_data(JNIEnv *env,jobject obj,jint id, jbyteArray d)  __attribute__((visibility("default")));

JNIEXPORT void JNICALL Java_com_reicast_emulator_emu_JNIdc_rendinit(JNIEnv *env,jobject obj,jint w,jint h)  __attribute__((visibility("default")));
JNIEXPORT jboolean JNICALL Java_com_reicast_emulator_emu_JNIdc_rendframe(JNIEnv *env,jobject obj)  __attribute__((visibility("default")));

JNIEXPORT void JNICALL Java_com_reicast_emulator_emu_JNIdc_vjoy(JNIEnv * env, jobject obj,u32 id,float x, float y, float w, float h)  __attribute__((visibility("default")));

JNIEXPORT void JNICALL Java_com_reicast_emulator_emu_JNIdc_initControllers(JNIEnv *env, jobject obj, jbooleanArray controllers, jobjectArray peripherals)  __attribute__((visibility("default")));

JNIEXPORT void JNICALL Java_com_reicast_emulator_emu_JNIdc_setupMic(JNIEnv *env,jobject obj,jobject sip)  __attribute__((visibility("default")));
JNIEXPORT void JNICALL Java_com_reicast_emulator_emu_JNIdc_diskSwap(JNIEnv *env,jobject obj,jstring disk)  __attribute__((visibility("default")));
JNIEXPORT void JNICALL Java_com_reicast_emulator_emu_JNIdc_vmuSwap(JNIEnv *env,jobject obj)  __attribute__((visibility("default")));
JNIEXPORT void JNICALL Java_com_reicast_emulator_emu_JNIdc_setupVmu(JNIEnv *env,jobject obj,jobject sip)  __attribute__((visibility("default")));

SETTINGS_ACCESSORS(Dynarec, dynarec.Enable, jboolean)
SETTINGS_ACCESSORS(Idleskip, dynarec.idleskip, jboolean)
SETTINGS_ACCESSORS(Unstable, dynarec.unstable_opt, jboolean)
SETTINGS_ACCESSORS(Safemode, dynarec.safemode, jboolean)
SETTINGS_ACCESSORS(Cable, dreamcast.cable, jint)
SETTINGS_ACCESSORS(Region, dreamcast.region, jint)
SETTINGS_ACCESSORS(Broadcast, dreamcast.broadcast, jint)
SETTINGS_ACCESSORS(Language, dreamcast.language, jint)
SETTINGS_ACCESSORS(Limitfps, aica.LimitFPS, jboolean)
SETTINGS_ACCESSORS(Nobatch, aica.NoBatch, jboolean)
SETTINGS_ACCESSORS(Nosound, aica.NoSound, jboolean)
SETTINGS_ACCESSORS(Mipmaps, rend.UseMipmaps, jboolean)
SETTINGS_ACCESSORS(Widescreen, rend.WideScreen, jboolean)
SETTINGS_ACCESSORS(Frameskip, pvr.ta_skip, jint)
SETTINGS_ACCESSORS(Pvrrender, pvr.rend, jint)
SETTINGS_ACCESSORS(Syncedrender, pvr.SynchronousRender, jboolean)
SETTINGS_ACCESSORS(Modvols, rend.ModifierVolumes, jboolean)
SETTINGS_ACCESSORS(Clipping, rend.Clipping, jboolean)
SETTINGS_ACCESSORS(Usereios, bios.UseReios, jboolean)
SETTINGS_ACCESSORS(Customtextures, rend.CustomTextures, jboolean)
SETTINGS_ACCESSORS(Showfps, rend.ShowFPS, jboolean)
SETTINGS_ACCESSORS(RenderToTextureBuffer, rend.RenderToTextureBuffer, jboolean)
SETTINGS_ACCESSORS(RenderToTextureUpscale, rend.RenderToTextureUpscale, jint)
SETTINGS_ACCESSORS(TextureUpscale, rend.TextureUpscale, jint)
SETTINGS_ACCESSORS(MaxFilteredTextureSize, rend.MaxFilteredTextureSize, jint)
SETTINGS_ACCESSORS(MaxThreads, pvr.MaxThreads, jint)

JNIEXPORT jint JNICALL Java_com_reicast_emulator_emu_JNIdc_getBootdisk(JNIEnv *env,jobject obj)  __attribute__((visibility("default")));
JNIEXPORT void JNICALL Java_com_reicast_emulator_emu_JNIdc_bootdisk(JNIEnv *env,jobject obj, jstring disk)  __attribute__((visibility("default")));

JNIEXPORT void JNICALL Java_com_reicast_emulator_emu_JNIdc_screenDpi(JNIEnv *env,jobject obj, jint screenDpi)  __attribute__((visibility("default")));
JNIEXPORT void JNICALL Java_com_reicast_emulator_emu_JNIdc_guiOpenSettings(JNIEnv *env,jobject obj)  __attribute__((visibility("default")));
JNIEXPORT jboolean JNICALL Java_com_reicast_emulator_emu_JNIdc_guiIsOpen(JNIEnv *env,jobject obj)  __attribute__((visibility("default")));

JNIEXPORT void JNICALL Java_com_reicast_emulator_periph_InputDeviceManager_joystickAdded(JNIEnv *env, jobject obj, jint id, jstring name)  __attribute__((visibility("default")));
JNIEXPORT void JNICALL Java_com_reicast_emulator_periph_InputDeviceManager_joystickRemoved(JNIEnv *env, jobject obj, jint id)  __attribute__((visibility("default")));
JNIEXPORT void JNICALL Java_com_reicast_emulator_periph_InputDeviceManager_virtualGamepadEvent(JNIEnv *env, jobject obj, jint kcode, jint joyx, jint joyy, jint lt, jint rt)  __attribute__((visibility("default")));
JNIEXPORT jboolean JNICALL Java_com_reicast_emulator_periph_InputDeviceManager_joystickButtonEvent(JNIEnv *env, jobject obj, jint id, jint key, jboolean pressed)  __attribute__((visibility("default")));
JNIEXPORT jboolean JNICALL Java_com_reicast_emulator_periph_InputDeviceManager_joystickAxisEvent(JNIEnv *env, jobject obj, jint id, jint key, jint value) __attribute__((visibility("default")));
JNIEXPORT void JNICALL Java_com_reicast_emulator_periph_InputDeviceManager_mouseEvent(JNIEnv *env, jobject obj, jint xpos, jint ypos, jint buttons) __attribute__((visibility("default")));
};

JNIEXPORT void JNICALL Java_com_reicast_emulator_emu_JNIdc_screenDpi(JNIEnv *env,jobject obj, jint screenDpi)
{
    screen_dpi = screenDpi;
}

void egl_stealcntx();
void SetApplicationPath(wchar *path);
void reios_init(int argc,wchar* argv[]);
int dc_init();
void dc_run();
void dc_pause();
void dc_term();

bool VramLockedWrite(u8* address);

bool rend_single_frame();
bool gles_init();

//extern cResetEvent rs,re;
extern int screen_width,screen_height;

static u64 tvs_base;
static char gamedisk[256];

// Additonal controllers 2, 3 and 4 connected ?
static bool add_controllers[3] = { false, false, false };
int **controller_periphs;

u16 kcode[4] = { 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF };
u32 vks[4];
s8 joyx[4],joyy[4];
u8 rt[4],lt[4];
float vjoy_pos[14][8];

extern s32 mo_x_abs;
extern s32 mo_y_abs;
extern u32 mo_buttons;

extern bool print_stats;



void os_DoEvents()
{
    // @@@ Nothing here yet
}

//
// Native thread that runs the actual nullDC emulator
//
static void *ThreadHandler(void *UserData)
{
    char *Args[3];
    const char *P;

    // Make up argument list
    P       = (const char *)UserData;
    Args[0] = (char*)"dc";
    Args[1] = (char*)"-config";
    Args[2] = P&&P[0]? (char *)malloc(strlen(P)+32):0;

    if(Args[2])
    {
        strcpy(Args[2],"config:image=");
        strcat(Args[2],P);
    }

    // Run nullDC emulator
    reios_init(Args[2]? 3:1,Args);
    return 0;
}

//
// Platform-specific NullDC functions
//


void UpdateInputState(u32 Port)
{
    // @@@ Nothing here yet
}

void UpdateVibration(u32 port, u32 value)
{

}

void *libPvr_GetRenderTarget()
{
    // No X11 window in Android
    return(0);
}

void *libPvr_GetRenderSurface()
{
    // No X11 display in Android
    return(0);
}

void common_linux_setup();

MapleDeviceType GetMapleDeviceType(int value)
{
    switch (value)
    {
        case 1:
            return MDT_SegaVMU;
        case 2:
            return MDT_Microphone;
        case 3:
            return MDT_PurupuruPack;
        default:
            return MDT_None;
    }
}

void os_SetupInput()
{
#if DC_PLATFORM == DC_PLATFORM_DREAMCAST
    // Create first controller
	settings.input.maple_devices[0] = MDT_SegaController;
	settings.input.maple_expansion_devices[0][0] = MDT_SegaVMU;
	settings.input.maple_expansion_devices[0][1] = GetMapleDeviceType(controller_periphs[0][1]);

    // Add additional controllers
    for (int i = 1; i < 4; i++)
    {
        if (add_controllers[i - 1])
        {
        	settings.input.maple_devices[i] = MDT_SegaController;
        	settings.input.maple_expansion_devices[i][0] = GetMapleDeviceType(controller_periphs[i][0]);
        	settings.input.maple_expansion_devices[i][1] = GetMapleDeviceType(controller_periphs[i][1]);
        }
        else
        	settings.input.maple_devices[i] = MDT_None;
    }
    mcfg_CreateDevices();
#endif
}

void os_SetWindowText(char const *Text)
{
    putinf("%s",Text);
}
JNIEXPORT void JNICALL Java_com_reicast_emulator_emu_JNIdc_config(JNIEnv *env,jobject obj,jstring dirName)
{
    // Set home directory based on User config
    const char* D = dirName? env->GetStringUTFChars(dirName,0):0;
    set_user_config_dir(D);
    set_user_data_dir(D);
    printf("Config dir is: %s\n", get_writable_config_path("/").c_str());
    printf("Data dir is:   %s\n", get_writable_data_path("/").c_str());
    env->ReleaseStringUTFChars(dirName,D);
}

JNIEXPORT void JNICALL Java_com_reicast_emulator_emu_JNIdc_bootdisk(JNIEnv *env,jobject obj, jstring disk) {
    if (disk != NULL) {
        settings.imgread.LoadDefaultImage = true;
        const char *P = env->GetStringUTFChars(disk, 0);
        if (!P) settings.imgread.DefaultImage[0] = '\0';
        else {
            printf("Boot Disk URI: '%s'\n", P);
            strncpy(settings.imgread.DefaultImage,(strlen(P)>=7)&&!memcmp(
                    P,"file://",7)? P+7:P,sizeof(settings.imgread.DefaultImage));
            settings.imgread.DefaultImage[sizeof(settings.imgread.DefaultImage) - 1] = '\0';
        }
    }
}

JNIEXPORT void JNICALL Java_com_reicast_emulator_emu_JNIdc_init(JNIEnv *env,jobject obj,jstring fileName)
{
    // Get filename string from Java
    const char* P = fileName ? env->GetStringUTFChars(fileName,0) : 0;
    if (!P) gamedisk[0] = '\0';
    else
    {
        printf("Game Disk URI: '%s'\n",P);
        strncpy(gamedisk,(strlen(P)>=7)&&!memcmp(P,"file://",7)? P+7:P,sizeof(gamedisk));
        gamedisk[sizeof(gamedisk)-1] = '\0';
        env->ReleaseStringUTFChars(fileName,P);
    }

    // Initialize platform-specific stuff
    common_linux_setup();

    // Set configuration
    settings.profile.run_counts = 0;


/*
  // Start native thread
  pthread_attr_init(&PTAttr);
  pthread_attr_setdetachstate(&PTAttr,PTHREAD_CREATE_DETACHED);
  pthread_create(&PThread,&PTAttr,ThreadHandler,CurFileName);
  pthread_attr_destroy(&PTAttr);
  */

    ThreadHandler(gamedisk);
}

JNIEXPORT void JNICALL Java_com_reicast_emulator_emu_JNIdc_diskSwap(JNIEnv *env,jobject obj,jstring disk)
{
    if (settings.imgread.LoadDefaultImage) {
        strncpy(settings.imgread.DefaultImage, gamedisk, sizeof(settings.imgread.DefaultImage));
        settings.imgread.DefaultImage[sizeof(settings.imgread.DefaultImage) - 1] = '\0';
        DiscSwap();
    } else if (disk != NULL) {
        settings.imgread.LoadDefaultImage = true;
        const char *P = env->GetStringUTFChars(disk, 0);
        if (!P) settings.imgread.DefaultImage[0] = '\0';
        else {
            printf("Swap Disk URI: '%s'\n", P);
            strncpy(settings.imgread.DefaultImage,(strlen(P)>=7)&&!memcmp(
                    P,"file://",7)? P+7:P,sizeof(settings.imgread.DefaultImage));
            settings.imgread.DefaultImage[sizeof(settings.imgread.DefaultImage) - 1] = '\0';
            env->ReleaseStringUTFChars(disk, P);
        }
        DiscSwap();
    }
}

#define SAMPLE_COUNT 512

JNIEnv* jenv; //we are abusing the f*** out of this poor guy
//JavaVM* javaVM = NULL; //this seems like the right way to go
//stuff for audio
jshortArray jsamples;
jmethodID writemid;
jmethodID coreMessageMid;
jmethodID dieMid;
jobject emu;
//stuff for microphone
jobject sipemu;
jmethodID getmicdata;
//stuff for vmu lcd
jobject vmulcd = NULL;
jbyteArray jpix = NULL;
jmethodID updatevmuscreen;
//stuff for saving prefs
JavaVM* g_jvm;
jobject g_emulator;
jmethodID saveSettingsMid;

JNIEXPORT void JNICALL Java_com_reicast_emulator_emu_JNIdc_query(JNIEnv *env,jobject obj,jobject emu_thread, jobject emulator)
{
    jmethodID reiosInfoMid=env->GetMethodID(env->GetObjectClass(emu_thread),"reiosInfo","(Ljava/lang/String;Ljava/lang/String;)V");

    char *id = (char*)malloc(11);
    strcpy(id, reios_disk_id());
    jstring reios_id = env->NewStringUTF(id);

    char *name = (char*)malloc(129);
    strcpy(name, reios_software_name);
    jstring reios_name = env->NewStringUTF(name);

    env->CallVoidMethod(emu_thread, reiosInfoMid, reios_id, reios_name);

    env->GetJavaVM(&g_jvm);
    g_emulator = env->NewGlobalRef(emulator);
    saveSettingsMid = env->GetMethodID(env->GetObjectClass(emulator), "SaveSettings", "()V");

    dc_init();
}

JNIEXPORT void JNICALL Java_com_reicast_emulator_emu_JNIdc_run(JNIEnv *env,jobject obj,jobject emu_thread)
{
    install_prof_handler(0);

    jenv=env;
    emu=emu_thread;

    jsamples=env->NewShortArray(SAMPLE_COUNT*2);
    writemid=env->GetMethodID(env->GetObjectClass(emu),"WriteBuffer","([SI)I");
    coreMessageMid=env->GetMethodID(env->GetObjectClass(emu),"coreMessage","([B)I");
    dieMid=env->GetMethodID(env->GetObjectClass(emu),"Die","()V");

    dc_run();
}

int msgboxf(const wchar* text,unsigned int type,...) {
    va_list args;

    wchar temp[2048];
    va_start(args, type);
    vsprintf(temp, text, args);
    va_end(args);

    int byteCount = strlen(temp);
    jbyteArray bytes = jenv->NewByteArray(byteCount);
    jenv->SetByteArrayRegion(bytes, 0, byteCount, (jbyte *) temp);

    return jenv->CallIntMethod(emu, coreMessageMid, bytes);
}

JNIEXPORT void JNICALL Java_com_reicast_emulator_emu_JNIdc_setupMic(JNIEnv *env,jobject obj,jobject sip)
{
    sipemu = env->NewGlobalRef(sip);
    getmicdata = env->GetMethodID(env->GetObjectClass(sipemu),"getData","()[B");
}

JNIEXPORT void JNICALL Java_com_reicast_emulator_emu_JNIdc_setupVmu(JNIEnv *env,jobject obj,jobject vmu)
{
    //env->GetJavaVM(&javaVM);
    vmulcd = env->NewGlobalRef(vmu);
    updatevmuscreen = env->GetMethodID(env->GetObjectClass(vmu),"updateBytes","([B)V");
    //jpix=env->NewByteArray(1536);
}

JNIEXPORT void JNICALL Java_com_reicast_emulator_emu_JNIdc_pause(JNIEnv *env,jobject obj)
{
    dc_pause();
}

JNIEXPORT void JNICALL Java_com_reicast_emulator_emu_JNIdc_destroy(JNIEnv *env,jobject obj)
{
    dc_term();
}

JNIEXPORT void JNICALL Java_com_reicast_emulator_emu_JNIdc_vmuSwap(JNIEnv *env,jobject obj)
{
    maple_device* olda = MapleDevices[0][0];
    maple_device* oldb = MapleDevices[0][1];
    MapleDevices[0][0] = NULL;
    MapleDevices[0][1] = NULL;
    usleep(50000);//50 ms, wait for host to detect disconnect

    MapleDevices[0][0] = oldb;
    MapleDevices[0][1] = olda;
}

JNIEXPORT jint JNICALL Java_com_reicast_emulator_emu_JNIdc_send(JNIEnv *env,jobject obj,jint cmd, jint param)
{
    if (cmd==0)
    {
        if (param==0)
        {
            KillTex=true;
            printf("Killing texture cache\n");
        }

        if (param==1)
        {
            settings.pvr.ta_skip^=1;
            printf("settings.pvr.ta_skip: %d\n",settings.pvr.ta_skip);
        }
        if (param==2)
        {
#if FEAT_SHREC != DYNAREC_NONE
            print_stats=true;
            printf("Storing blocks ...\n");
#endif
        }
    }
    else if (cmd==1)
    {
        if (param==0)
            sample_Stop();
        else
            sample_Start(param);
    }
    else if (cmd==2)
    {
    }
    return 0;
}

JNIEXPORT jint JNICALL Java_com_reicast_emulator_emu_JNIdc_data(JNIEnv *env, jobject obj, jint id, jbyteArray d)
{
    if (id==1)
    {
        printf("Loading symtable (%p,%p,%d,%p)\n",env,obj,id,d);
        jsize len=env->GetArrayLength(d);
        u8* syms=(u8*)malloc((size_t)len);
        printf("Loading symtable to %8s, %d\n",syms,len);
        env->GetByteArrayRegion(d,0,len,(jbyte*)syms);
        sample_Syms(syms, (size_t)len);
    }
    return 0;
}


JNIEXPORT jboolean JNICALL Java_com_reicast_emulator_emu_JNIdc_rendframe(JNIEnv *env,jobject obj)
{
    return (jboolean)rend_single_frame();
}

JNIEXPORT void JNICALL Java_com_reicast_emulator_emu_JNIdc_rendinit(JNIEnv * env, jobject obj, jint w,jint h)
{
    screen_width  = w;
    screen_height = h;

    //gles_term();

    egl_stealcntx();

    if (!gles_init())
    die("OPENGL FAILED");

    install_prof_handler(1);
}

JNIEXPORT void JNICALL Java_com_reicast_emulator_emu_JNIdc_vjoy(JNIEnv * env, jobject obj,u32 id,float x, float y, float w, float h)
{
    if(id<sizeof(vjoy_pos)/sizeof(vjoy_pos[0]))
    {
        vjoy_pos[id][0] = x;
        vjoy_pos[id][1] = y;
        vjoy_pos[id][2] = w;
        vjoy_pos[id][3] = h;
    }
}

JNIEXPORT void JNICALL Java_com_reicast_emulator_emu_JNIdc_initControllers(JNIEnv *env, jobject obj, jbooleanArray controllers, jobjectArray peripherals)
{
    jboolean *controllers_body = env->GetBooleanArrayElements(controllers, 0);
    memcpy(add_controllers, controllers_body, 3);
    env->ReleaseBooleanArrayElements(controllers, controllers_body, 0);

    int obj_len = env->GetArrayLength(peripherals);
    jintArray port = (jintArray) env->GetObjectArrayElement(peripherals, 0);
    int port_len = env->GetArrayLength(port);
    controller_periphs = new int*[obj_len];
    for (int i = 0; i < obj_len; ++i) {
        port = (jintArray) env->GetObjectArrayElement(peripherals, i);
        jint *items = env->GetIntArrayElements(port, 0);
        controller_periphs[i] = new int[port_len];
        for (int j = 0; j < port_len; ++j) {
            controller_periphs[i][j]= items[j];
        }
    }
    for (int i = 0; i < obj_len; i++) {
        jintArray port = (jintArray) env->GetObjectArrayElement(peripherals, i);
        jint *items = env->GetIntArrayElements(port, 0);
        env->ReleaseIntArrayElements(port, items, 0);
        env->DeleteLocalRef(port);
    }
}

JNIEXPORT void JNICALL Java_com_reicast_emulator_emu_JNIdc_guiOpenSettings(JNIEnv *env, jobject obj)
{
    gui_open_settings();
}

JNIEXPORT jboolean JNICALL Java_com_reicast_emulator_emu_JNIdc_guiIsOpen(JNIEnv *env, jobject obj)
{
    return gui_is_open();
}

// Audio Stuff
u32 androidaudio_push(void* frame, u32 amt, bool wait)
{
    verify(amt==SAMPLE_COUNT);
    //yeah, do some audio piping magic here !
    jenv->SetShortArrayRegion(jsamples,0,amt*2,(jshort*)frame);
    return jenv->CallIntMethod(emu,writemid,jsamples,wait);
}

void androidaudio_init()
{
    // Nothing to do here...
}

void androidaudio_term()
{
    // Move along, there is nothing to see here!
}

bool os_IsAudioBuffered()
{
    return jenv->CallIntMethod(emu,writemid,jsamples,-1)==0;
}

audiobackend_t audiobackend_android = {
        "android", // Slug
        "Android Audio", // Name
        &androidaudio_init,
        &androidaudio_push,
        &androidaudio_term
};

int get_mic_data(u8* buffer)
{
    jbyteArray jdata = (jbyteArray)jenv->CallObjectMethod(sipemu,getmicdata);
    if(jdata==NULL){
        //LOGW("get_mic_data NULL");
        return 0;
    }
    jenv->GetByteArrayRegion(jdata, 0, SIZE_OF_MIC_DATA, (jbyte*)buffer);
    jenv->DeleteLocalRef(jdata);
    return 1;
}

int push_vmu_screen(u8* buffer)
{
    if(vmulcd==NULL){
        return 0;
    }
    JNIEnv *env = jenv;
    //javaVM->AttachCurrentThread(&env, NULL);
    if(jpix==NULL){
        jpix=env->NewByteArray(1536);
    }
    env->SetByteArrayRegion(jpix,0,1536,(jbyte*)buffer);
    env->CallVoidMethod(vmulcd,updatevmuscreen,jpix);
    return 1;
}

void os_DebugBreak()
{
    // TODO: notify the parent thread about it ...

	raise(SIGSTOP);
	
    // Attach debugger here to figure out what went wrong
    for(;;) ;
}

void SaveSettings()
{
    if (g_jvm == NULL)
        return;

    JNIEnv *env;
    bool detach_thread = false;
    int rc = g_jvm->GetEnv((void **)&env, JNI_VERSION_1_6);
    if (rc  == JNI_EDETACHED) {
        if (g_jvm->AttachCurrentThread(&env, NULL) != 0)
            // abort
            return;
        detach_thread = true;
    }
    else if (rc == JNI_EVERSION)
        // abort
        return;

    env->CallVoidMethod(g_emulator, saveSettingsMid);

    if (detach_thread)
        g_jvm->DetachCurrentThread();
}

JNIEXPORT void JNICALL Java_com_reicast_emulator_periph_InputDeviceManager_joystickAdded(JNIEnv *env, jobject obj, jint id, jstring name)
{
    const char* joyname = env->GetStringUTFChars(name,0);
    new AndroidGamepadDevice(0, id, joyname);
    env->ReleaseStringUTFChars(name, joyname);

}
JNIEXPORT void JNICALL Java_com_reicast_emulator_periph_InputDeviceManager_joystickRemoved(JNIEnv *env, jobject obj, jint id)
{
    AndroidGamepadDevice *device = AndroidGamepadDevice::GetAndroidGamepad(id);
    if (device != NULL)
    	delete device;
}

JNIEXPORT void JNICALL Java_com_reicast_emulator_periph_InputDeviceManager_virtualGamepadEvent(JNIEnv *env, jobject obj, jint kcode, jint joyx, jint joyy, jint lt, jint rt)
{
    AndroidGamepadDevice *device = AndroidGamepadDevice::GetAndroidGamepad(AndroidGamepadDevice::VIRTUAL_GAMEPAD_ID);
    if (device != NULL)
        device->virtual_gamepad_event(kcode, joyx, joyy, lt, rt);
}

JNIEXPORT jboolean JNICALL Java_com_reicast_emulator_periph_InputDeviceManager_joystickButtonEvent(JNIEnv *env, jobject obj, jint id, jint key, jboolean pressed)
{
    AndroidGamepadDevice *device = AndroidGamepadDevice::GetAndroidGamepad(id);
    if (device != NULL)
        return device->gamepad_btn_input(key, pressed);
    else
    	return false;

}
JNIEXPORT jboolean JNICALL Java_com_reicast_emulator_periph_InputDeviceManager_joystickAxisEvent(JNIEnv *env, jobject obj, jint id, jint key, jint value)
{
    AndroidGamepadDevice *device = AndroidGamepadDevice::GetAndroidGamepad(id);
    if (device != NULL)
    	return device->gamepad_axis_input(key, value);
    else
    	return false;
}

JNIEXPORT void JNICALL Java_com_reicast_emulator_periph_InputDeviceManager_mouseEvent(JNIEnv *env, jobject obj, jint xpos, jint ypos, jint buttons)
{
    mo_x_abs = xpos;
    mo_y_abs = ypos;
    mo_buttons = 0xFFFF;
    if (buttons & 1)	// Left
    	mo_buttons &= ~4;
    if (buttons & 2)	// Right
    	mo_buttons &= ~2;
    if (buttons & 4)	// Middle
    	mo_buttons &= ~8;
    mouse_gamepad.gamepad_btn_input(1, (buttons & 1) != 0);
    mouse_gamepad.gamepad_btn_input(2, (buttons & 2) != 0);
    mouse_gamepad.gamepad_btn_input(4, (buttons & 4) != 0);
}
