/* DO NOT EDIT THIS FILE - it is machine generated */
#include <jni.h>
/* Header for class cnmat_sdif_SDIFBuffer */

#ifndef _Included_cnmat_sdif_SDIFBuffer
#define _Included_cnmat_sdif_SDIFBuffer
#ifdef __cplusplus
extern "C" {
#endif
/*
 * Class:     cnmat_sdif_SDIFBuffer
 * Method:    n_getMatrix
 * Signature: (Ljava/lang/String;[CDI)Lcnmat/sdif/SDIFMatrix;
 */
JNIEXPORT jobject JNICALL Java_cnmat_sdif_SDIFBuffer_n_1getMatrix
  (JNIEnv *, jclass, jstring, jcharArray, jdouble, jint);

/*
 * Class:     cnmat_sdif_SDIFBuffer
 * Method:    n_getMatrixHeader
 * Signature: (Ljava/lang/String;)Lcnmat/sdif/SDIFMatrixHeader;
 */
JNIEXPORT jobject JNICALL Java_cnmat_sdif_SDIFBuffer_n_1getMatrixHeader
  (JNIEnv *, jclass, jstring);

/*
 * Class:     cnmat_sdif_SDIFBuffer
 * Method:    n_init
 * Signature: ()I
 */
JNIEXPORT jint JNICALL Java_cnmat_sdif_SDIFBuffer_n_1init
  (JNIEnv *, jclass);

#ifdef __cplusplus
}
#endif
#endif
