/* DO NOT EDIT THIS FILE - it is machine generated */
#include <jni.h>
/* Header for class com_dialectek_blackguard_Scene */

#ifndef _Included_com_dialectek_blackguard_Scene
#define _Included_com_dialectek_blackguard_Scene
#ifdef __cplusplus
extern "C" {
#endif
#undef com_dialectek_blackguard_Scene_UNIT_SIZE
#define com_dialectek_blackguard_Scene_UNIT_SIZE 5.0f
#undef com_dialectek_blackguard_Scene_DEATH_ANIMATION_DURATION
#define com_dialectek_blackguard_Scene_DEATH_ANIMATION_DURATION 250i64
#undef com_dialectek_blackguard_Scene_HIT_ANIMATION_DURATION
#define com_dialectek_blackguard_Scene_HIT_ANIMATION_DURATION 100i64
#undef com_dialectek_blackguard_Scene_HIT_RECOIL_ANGLE
#define com_dialectek_blackguard_Scene_HIT_RECOIL_ANGLE 45.0f
#undef com_dialectek_blackguard_Scene_FRUSTUM_NEAR
#define com_dialectek_blackguard_Scene_FRUSTUM_NEAR 1.0f
#undef com_dialectek_blackguard_Scene_FRUSTUM_FAR
#define com_dialectek_blackguard_Scene_FRUSTUM_FAR 250.0f
#undef com_dialectek_blackguard_Scene_VIEW_TRANSITION_DURATION
#define com_dialectek_blackguard_Scene_VIEW_TRANSITION_DURATION 200i64
/*
 * Class:     com_dialectek_blackguard_Scene
 * Method:    disposeAnimations
 * Signature: ()Z
 */
JNIEXPORT jboolean JNICALL Java_com_dialectek_blackguard_Scene_disposeAnimations
  (JNIEnv *, jobject);

/*
 * Class:     com_dialectek_blackguard_Scene
 * Method:    getAnimator
 * Signature: ()[I
 */
JNIEXPORT jintArray JNICALL Java_com_dialectek_blackguard_Scene_getAnimator
  (JNIEnv *, jobject);

#ifdef __cplusplus
}
#endif
#endif