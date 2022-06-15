/* DO NOT EDIT THIS FILE - it is machine generated */
#include <jni.h>
/* Header for class blackguard_Scene */

#ifndef _Included_blackguard_Scene
#define _Included_blackguard_Scene
#ifdef __cplusplus
extern "C" {
#endif
#undef blackguard_Scene_FOCUS_TRAVERSABLE_UNKNOWN
#define blackguard_Scene_FOCUS_TRAVERSABLE_UNKNOWN 0L
#undef blackguard_Scene_FOCUS_TRAVERSABLE_DEFAULT
#define blackguard_Scene_FOCUS_TRAVERSABLE_DEFAULT 1L
#undef blackguard_Scene_FOCUS_TRAVERSABLE_SET
#define blackguard_Scene_FOCUS_TRAVERSABLE_SET 2L
#undef blackguard_Scene_TOP_ALIGNMENT
#define blackguard_Scene_TOP_ALIGNMENT 0.0f
#undef blackguard_Scene_CENTER_ALIGNMENT
#define blackguard_Scene_CENTER_ALIGNMENT 0.5f
#undef blackguard_Scene_BOTTOM_ALIGNMENT
#define blackguard_Scene_BOTTOM_ALIGNMENT 1.0f
#undef blackguard_Scene_LEFT_ALIGNMENT
#define blackguard_Scene_LEFT_ALIGNMENT 0.0f
#undef blackguard_Scene_RIGHT_ALIGNMENT
#define blackguard_Scene_RIGHT_ALIGNMENT 1.0f
#undef blackguard_Scene_serialVersionUID
#define blackguard_Scene_serialVersionUID -7644114512714619750i64
#undef blackguard_Scene_serialVersionUID
#define blackguard_Scene_serialVersionUID -2284879212465893870i64
#undef blackguard_Scene_UNIT_SIZE
#define blackguard_Scene_UNIT_SIZE 0.05f
#undef blackguard_Scene_DEATH_ANIMATION_DURATION
#define blackguard_Scene_DEATH_ANIMATION_DURATION 250i64
#undef blackguard_Scene_HIT_ANIMATION_DURATION
#define blackguard_Scene_HIT_ANIMATION_DURATION 100i64
#undef blackguard_Scene_HIT_RECOIL_ANGLE
#define blackguard_Scene_HIT_RECOIL_ANGLE 45.0f
#undef blackguard_Scene_FRUSTUM_ANGLE
#define blackguard_Scene_FRUSTUM_ANGLE 60.0f
#undef blackguard_Scene_FRUSTUM_NEAR
#define blackguard_Scene_FRUSTUM_NEAR 0.01f
#undef blackguard_Scene_FRUSTUM_FAR
#define blackguard_Scene_FRUSTUM_FAR 100.0f
#undef blackguard_Scene_VIEW_TRANSITION_DURATION
#define blackguard_Scene_VIEW_TRANSITION_DURATION 200i64
/*
 * Class:     blackguard_Scene
 * Method:    disposeAnimations
 * Signature: ()Z
 */
JNIEXPORT jboolean JNICALL Java_blackguard_Scene_disposeAnimations
  (JNIEnv *, jobject);

/*
 * Class:     blackguard_Scene
 * Method:    getAnimator
 * Signature: ()[I
 */
JNIEXPORT jintArray JNICALL Java_blackguard_Scene_getAnimator
  (JNIEnv *, jobject);

#ifdef __cplusplus
}
#endif
#endif