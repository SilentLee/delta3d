#ifndef DELTA_SCENE_HIERARCHY_EDITOR_EXPORT_FOR_MS_WIN
#define DELTA_SCENE_HIERARCHY_EDITOR_EXPORT_FOR_MS_WIN

/**
* This is modeled from the DT_EXPORT macro found in dtCore/export.h.  
* We define another due to conflicts with using the DT_EXPORT while  
* trying to import Delta3D symbols.  The STAGE_MAP_HIERARCHY_EDITOR_EXPORT macro should be used
* in front of any classes that are to be exported from the testGameActorLibrary.
* Also note that STAGE_MAP_HIERARCHY_EDITOR_PLUGIN should be defined in the compiler 
* preprocessor #defines.
*/
#if defined(_MSC_VER) || defined(__CYGWIN__) || defined(__MINGW32__) || defined( __BCPLUSPLUS__)  || defined( __MWERKS__)
#  ifdef DELTA_SCENE_HIERARCHY_EDITOR_PLUGIN
#    define DELTA_SCENE_HIERARCHY_EDITOR_EXPORT __declspec(dllexport)
#  else
#    define DELTA_SCENE_HIERARCHY_EDITOR_EXPORT __declspec(dllimport)
#  endif
#else
#  ifdef DELTA_SCENE_HIERARCHY_EDITOR_PLUGIN
#    define DELTA_SCENE_HIERARCHY_EDITOR_EXPORT  __attribute__ ((visibility("default")))
#  else
#    define DELTA_SCENE_HIERARCHY_EDITOR_EXPORT
#  endif
#endif

#endif
