/******************************************************************************/
#include "stdafx.h"
#include "../Shaders/!Header CPU.h"
namespace EE{
#include "Shader Compiler.h"
/******************************************************************************/
#define BUMP_MAPPING   1
#define MULTI_MATERIAL 1

#if WINDOWS // DirectX 10+
   #define COMPILE_4  0
   #define COMPILE_GL 0
#endif

/**/
#define MAIN

/*#define DEFERRED
#define BLEND_LIGHT
#define FORWARD // Forward Shaders in OpenGL compile .. FIXME

#define AMBIENT
#define AMBIENT_OCCLUSION
#define BEHIND
#define BLEND
#define BONE_HIGHLIGHT
#define DEPTH_OF_FIELD*/
#define EARLY_Z
#define EFFECTS_2D
/*#define EFFECTS_3D*/
#define FOG_LOCAL
/*#define FUR
#define FXAA
#define HDR
#define LAYERED_CLOUDS
#define MOTION_BLUR
#define OVERLAY*/
#define POSITION
/*#define SET_COLOR
#define VOLUMETRIC_CLOUDS
#define VOLUMETRIC_LIGHTS
#define WATER
#define WORLD_EDITOR
/******************************************************************************
#define DX10_INPUT_LAYOUT
/******************************************************************************/
// SHADER TECHNIQUE NAMES
/******************************************************************************/
Str8 TechNameDeferred  (Int skin, Int materials, Int textures, Int bump_mode, Int alpha_test, Int detail, Int macro, Int rflct, Int color, Int mtrl_blend, Int heightmap, Int fx, Int tess) {return S8+skin+materials+textures+bump_mode+alpha_test+detail+macro+rflct+color+mtrl_blend+heightmap+fx+tess;}
Str8 TechNameForward   (Int skin, Int materials, Int textures, Int bump_mode, Int alpha_test, Int light_map, Int detail, Int rflct, Int color, Int mtrl_blend, Int heightmap, Int fx,   Int light_dir, Int light_dir_shd, Int light_dir_shd_num,   Int light_point, Int light_point_shd,   Int light_linear, Int light_linear_shd,   Int light_cone, Int light_cone_shd,   Int tess) {return S8+skin+materials+textures+bump_mode+alpha_test+light_map+detail+rflct+color+mtrl_blend+heightmap+fx+light_dir+light_dir_shd+light_dir_shd_num+light_point+light_point_shd+light_linear+light_linear_shd+light_cone+light_cone_shd+tess;}
Str8 TechNameBlendLight(Int skin, Int color    , Int textures, Int bump_mode, Int alpha_test, Int alpha, Int light_map, Int rflct, Int fx, Int per_pixel, Int shadow_maps) {return S8+skin+color+textures+bump_mode+alpha_test+alpha+light_map+rflct+fx+per_pixel+shadow_maps;}
Str8 TechNamePosition  (Int skin, Int textures, Int test_blend, Int fx, Int tess) {return S8+skin+textures+test_blend+fx+tess;}
Str8 TechNameBlend     (Int skin, Int color, Int rflct, Int textures) {return S8+skin+color+rflct+textures;}
Str8 TechNameSetColor  (Int skin, Int textures, Int tess) {return S8+skin+textures+tess;}
Str8 TechNameBehind    (Int skin, Int textures) {return S8+skin+textures;}
Str8 TechNameEarlyZ    (Int skin) {return S8+skin;}
Str8 TechNameAmbient   (Int skin, Int alpha_test, Int light_map) {return S8+skin+alpha_test+light_map;}
Str8 TechNameOverlay   (Int skin, Int normal) {return S8+skin+normal;}
Str8 TechNameFurBase   (Int skin, Int size, Int diffuse) {return S8+"Base"+skin+size+diffuse;}
Str8 TechNameFurSoft   (Int skin, Int size, Int diffuse) {return S8+"Soft"+skin+size+diffuse;}
Str8 TechNameTattoo    (Int skin, Int tess             ) {return S8+skin+tess;}
/******************************************************************************/
#if COMPILE_4 || COMPILE_GL
/******************************************************************************/
static Memx<ShaderCompiler> ShaderCompilers; // use 'Memx' because we store pointers to 'ShaderCompiler'
/******************************************************************************/
// FIXME remove this
// COMPILER
/******************************************************************************/
// compile
#if EE_PRIVATE
Bool ShaderCompileTry(C Str &src, C Str &dest, API api, SHADER_MODEL model, C MemPtr<ShaderMacro> &macros, C MemPtr<ShaderGLSL> &stg=null, Str *messages=null); // compile shader from 'src' file to 'dest' using additional 'macros', false on fail, 'messages'=optional parameter which will receive any messages that occurred during compilation
void ShaderCompile   (C Str &src, C Str &dest, API api, SHADER_MODEL model, C MemPtr<ShaderMacro> &macros, C MemPtr<ShaderGLSL> &stg                         ); // compile shader from 'src' file to 'dest' using additional 'macros', Exit  on fail
#endif
Bool ShaderCompileTry(C Str &src, C Str &dest, API api, SHADER_MODEL model, C MemPtr<ShaderMacro> &macros=null, Str *messages=null); // compile shader from 'src' file to 'dest' using additional 'macros', false on fail, 'messages'=optional parameter which will receive any messages that occurred during compilation
void ShaderCompile   (C Str &src, C Str &dest, API api, SHADER_MODEL model, C MemPtr<ShaderMacro> &macros=null                    ); // compile shader from 'src' file to 'dest' using additional 'macros', Exit  on fail

#define FORCE_LOG      0
struct OldShaderCompiler
{
   Str               src, dest;
   API               api;
   SHADER_MODEL      model;
   Memc<ShaderMacro> macros;
   Memc<ShaderGLSL > glsl;

   void compile()
   {
      Str messages; Bool ok=ShaderCompileTry(src, dest, api, model, macros, glsl, &messages);
      if(!ok || DEBUG || FORCE_LOG)if(messages.is())LogN(S+"Shader\n\""+src+"\"\nto file\n\""+dest+"\"\n"+messages);
      if(!ok)
      {
      #if !DX11
         if(api==API_DX)Exit("Can't compile DX10+ Shaders when not using DX10+ engine version");
      #endif
      #if !GL
         if(api==API_GL)Exit("Can't compile OpenGL Shaders when not using OpenGL engine version");
      #endif
         Exit(S+"Error compiling shader\n\""+src+"\"\nto file\n\""+dest+"\"."+(messages.is() ? S+"\n\nCompilation Messages:\n"+messages : S));
      }
   }
};
static Memc<OldShaderCompiler> OldShaderCompilers;
static void ThreadCompile(OldShaderCompiler &shader_compiler, Ptr user, Int thread_index)
{
   ThreadMayUseGPUData();
   shader_compiler.compile();
}
/******************************************************************************/
static void Add(C Str &src, C Str &dest, API api, SHADER_MODEL model, C MemPtr<ShaderGLSL> &glsl=null)
{
   OldShaderCompiler &sc=OldShaderCompilers.New();
   sc.src  =src  ;
   sc.dest =dest ;
   sc.api  =api  ;
   sc.model=model;
   sc.glsl =glsl ;
}
static void Add(C Str &src, C Str &dest, API api, SHADER_MODEL model, C Str &names, C MemPtr<ShaderGLSL> &glsl=null)
{
   OldShaderCompiler &sc=OldShaderCompilers.New();
   sc.src  =src  ;
   sc.dest =dest ;
   sc.api  =api  ;
   sc.model=model;
   sc.glsl =glsl ;
   sc.macros.New().set("CUSTOM_TECHNIQUE", names);
}
/******************************************************************************/
// SHADER TECHNIQUE DECLARATIONS
/******************************************************************************/
static Str TechDeferred(Int skin, Int materials, Int textures, Int bump_mode, Int alpha_test, Int detail, Int macro, Int rflct, Int color, Int mtrl_blend, Int heightmap, Int fx, Int tess)
{
   Str params=               S+skin+','+materials+','+textures+','+bump_mode+','+alpha_test+','+detail+','+macro+','+rflct+','+color+','+mtrl_blend+','+heightmap+','+fx+','+tess,
       name  =TechNameDeferred(skin  ,  materials  ,  textures  ,  bump_mode  ,  alpha_test  ,  detail  ,  macro  ,  rflct  ,  color  ,  mtrl_blend  ,  heightmap  ,  fx  ,  tess);
   return tess ? S+"TECHNIQUE_TESSELATION("+name+", VS("+params+"), PS("+params+"), HS("+params+"), DS("+params+"));"
               : S+"TECHNIQUE            ("+name+", VS("+params+"), PS("+params+")                                );";
}

static Str TechForward(Int skin, Int materials, Int textures, Int bump_mode, Int alpha_test, Int light_map, Int detail, Int rflct, Int color, Int mtrl_blend, Int heightmap, Int fx,   Int light_dir, Int light_dir_shd, Int light_dir_shd_num,   Int light_point, Int light_point_shd,   Int light_linear, Int light_linear_shd,   Int light_cone, Int light_cone_shd,   Int tess)
{
   Str params=              S+skin+','+materials+','+textures+','+bump_mode+','+alpha_test+','+light_map+','+detail+','+rflct+','+color+','+mtrl_blend+','+heightmap+','+fx+','   +light_dir+','+light_dir_shd+','+light_dir_shd_num+   ','+light_point+','+light_point_shd+   ','+light_linear+','+light_linear_shd+   ','+light_cone+','+light_cone_shd+','+tess,
       name  =TechNameForward(skin  ,  materials  ,  textures  ,  bump_mode  ,  alpha_test  ,  light_map  ,  detail  ,  rflct  ,  color  ,  mtrl_blend  ,  heightmap  ,  fx  ,     light_dir  ,  light_dir_shd  ,  light_dir_shd_num     ,  light_point  ,  light_point_shd     ,  light_linear  ,  light_linear_shd     ,  light_cone  ,  light_cone_shd  ,  tess);
   return tess ? S+"TECHNIQUE_TESSELATION("+name+", VS("+params+"), PS("+params+"), HS("+params+"), DS("+params+"));"
               : S+"TECHNIQUE            ("+name+", VS("+params+"), PS("+params+")                                );";
}
static Str TechForwardLight(Int skin, Int materials, Int textures, Int bump_mode, Int alpha_test, Int light_map, Int detail, Int rflct, Int color, Int mtrl_blend, Int heightmap, Int fx, Int tess)
{
   Str names;
   REPD(shd, 2)
   {
      if(shd)for(Int maps=2; maps<=6; maps+=2) // 2, 4, 6 maps
      names+=TechForward(skin, materials, textures, bump_mode, alpha_test, light_map, detail, rflct, color, mtrl_blend, heightmap, fx,   true ,shd  ,maps,  false,false,  false,false,  false,false,  tess);else // light dir with shadow maps
      names+=TechForward(skin, materials, textures, bump_mode, alpha_test, light_map, detail, rflct, color, mtrl_blend, heightmap, fx,   true ,false,   0,  false,false,  false,false,  false,false,  tess);     // light dir no   shadow
      names+=TechForward(skin, materials, textures, bump_mode, alpha_test, light_map, detail, rflct, color, mtrl_blend, heightmap, fx,   false,false,   0,  true ,shd  ,  false,false,  false,false,  tess);     // light point
      names+=TechForward(skin, materials, textures, bump_mode, alpha_test, light_map, detail, rflct, color, mtrl_blend, heightmap, fx,   false,false,   0,  false,false,  true ,shd  ,  false,false,  tess);     // light sqr
      names+=TechForward(skin, materials, textures, bump_mode, alpha_test, light_map, detail, rflct, color, mtrl_blend, heightmap, fx,   false,false,   0,  false,false,  false,false,  true ,shd  ,  tess);     // light cone
   }  names+=TechForward(skin, materials, textures, bump_mode, alpha_test, light_map, detail, rflct, color, mtrl_blend, heightmap, fx,   false,false,   0,  false,false,  false,false,  false,false,  tess);     // no light
   return names;
}

static Str TechBlend(Int skin, Int color, Int rflct, Int textures)
{
   Str params=            S+skin+','+color+','+rflct+','+textures,
       name  =TechNameBlend(skin  ,  color  ,  rflct  ,  textures);
   return S+"TECHNIQUE("+name+", VS("+params+"), PS("+params+"));";
}
static ShaderGLSL TechBlendGlsl(Int skin, Int color, Int rflct, Int textures)
{
   return ShaderGLSL().set("Main", TechNameBlend(skin, color, rflct, textures))
      .par("skin"    , skin    )
      .par("COLOR"   , color   )
      .par("rflct"   , rflct   )
      .par("textures", textures);
}

static Str TechBlendLight(Int skin, Int color, Int textures, Int bump_mode, Int alpha_test, Int alpha, Int light_map, Int rflct, Int fx, Int per_pixel, Int shadow_maps)
{
   Str params=                 S+skin+','+color+','+textures+','+bump_mode+','+alpha_test+','+alpha+','+light_map+','+rflct+','+fx+','+per_pixel+','+shadow_maps,
       name  =TechNameBlendLight(skin  ,  color  ,  textures  ,  bump_mode  ,  alpha_test  ,  alpha  ,  light_map  ,  rflct  ,  fx  ,  per_pixel  ,  shadow_maps);
   return S+"TECHNIQUE("+name+", VS("+params+"), PS("+params+"));";
}
static ShaderGLSL TechBlendLightGlsl(Int skin, Int color, Int textures, Int bump_mode, Int alpha_test, Int alpha, Int light_map, Int rflct, Int fx, Int per_pixel, Int shadow_maps)
{
   return ShaderGLSL().set("Main", TechNameBlendLight(skin, color, textures, bump_mode, alpha_test, alpha, light_map, rflct, fx, per_pixel, shadow_maps))
      .par("skin"       , skin       )
      .par("COLOR"      , color      )
      .par("textures"   , textures   )
      .par("bump_mode"  , bump_mode  )
      .par("alpha_test" , alpha_test )
      .par("ALPHA"      , alpha      )
      .par("light_map"  , light_map  )
      .par("rflct"      , rflct      )
      .par("fx"         , fx         )
      .par("per_pixel"  , per_pixel  )
      .par("shadow_maps", shadow_maps);
}

static Str TechSetColor(Int skin, Int textures, Int tess)
{
   Str params=               S+skin+','+textures+','+tess,
       name  =TechNameSetColor(skin  ,  textures  ,  tess);
   return tess ? S+"TECHNIQUE_TESSELATION("+name+", VS("+params+"), PS("+params+"), HS("+params+"), DS("+params+"));"
               : S+"TECHNIQUE            ("+name+", VS("+params+"), PS("+params+")                                );";
}

static Str TechBehind(Int skin, Int textures)
{
   Str params=             S+skin+','+textures,
       name  =TechNameBehind(skin  ,  textures);
   return S+"TECHNIQUE("+name+", VS("+params+"), PS("+params+"));";
}

static Str TechAmbient(Int skin, Int alpha_test, Int light_map)
{
   Str params=              S+skin+','+alpha_test+','+light_map,
       name  =TechNameAmbient(skin  ,  alpha_test  ,  light_map);
   return S+"TECHNIQUE("+name+", VS("+params+"), PS("+params+"));";
}

static Str TechOverlay(Int skin, Int normal)
{
   Str params=              S+skin+','+normal,
       name  =TechNameOverlay(skin  ,  normal);
   return S+"TECHNIQUE("+name+", VS("+params+"), PS("+params+"));";
}

static Str TechTattoo(Int skin, Int tess)
{
   Str params=             S+skin+','+tess,
       name  =TechNameTattoo(skin  ,  tess);
   return tess ? S+"TECHNIQUE_TESSELATION("+name+", VS("+params+"), PS("+params+"), HS("+params+"), DS("+params+"));"
               : S+"TECHNIQUE            ("+name+", VS("+params+"), PS("+params+")                                );";
}

static Str TechFurBase(Int skin, Int size, Int diffuse)
{
   Str params=              S+skin+','+size+','+diffuse,
       name  =TechNameFurBase(skin  ,  size  ,  diffuse);
   return S+"TECHNIQUE("+name+", Base_VS("+params+"), Base_PS("+params+"));";
}

static Str TechFurSoft(Int skin, Int size, Int diffuse)
{
   Str params=              S+skin+','+size+','+diffuse,
       name  =TechNameFurSoft(skin  ,  size  ,  diffuse);
   return S+"TECHNIQUE("+name+", Soft_VS("+params+"), Soft_PS("+params+"));";
}
/******************************************************************************/
// LISTING ALL SHADER TECHNIQUES
/******************************************************************************/
static void Compile(API api)
{
   if(!DataPath().is())Exit("Can't compile default shaders - 'DataPath' not specified");

   Str src,
       src_path=GetPath(GetPath(__FILE__))+"\\Shaders\\", // for example "C:/Esenthel/Engine/Src/Shaders/"
      dest_path=DataPath();                               // for example "C:/Esenthel/Data/Shader/4/"
   switch(api)
   {
      default: return;

      case API_GL: dest_path+="Shader\\GL\\"; break;
      case API_DX: dest_path+="Shader\\4\\" ; break;
   }
   FCreateDirs(dest_path);
   SHADER_MODEL model=SM_4;

   Bool ms  =(api==API_DX), // if support multi-sampling in shaders
        tess=(api==API_DX); // if support tesselation    in shaders

#ifdef MAIN
{
   ShaderCompiler &compiler=ShaderCompilers.New().set(dest_path+"Main", model, api);
   {
      ShaderCompiler::Source &src=compiler.New(src_path+"Main.cpp");
                     src.New("Draw2DFlat", "Draw2DFlat_VS", "DrawFlat_PS");
                     src.New("Draw3DFlat", "Draw3DFlat_VS", "DrawFlat_PS");
      if(api!=API_DX)src.New("SetCol"    , "Draw_VS"      , "DrawFlat_PS"); // this version fails on DX
      else           src.New("SetCol"    , "SetCol_VS"    , "SetCol_PS"  ); // THERE IS A BUG ON NVIDIA GEFORCE DX10+ when trying to clear normal render target using SetCol "Bool clear_nrm=(_nrm && !NRM_CLEAR_START && ClearNrm());", with D.depth2DOn(true) entire RT is cleared instead of background pixels only, this was verified on Windows 10 GeForce 650m, drivers 381, this version works OK on DX, TODO: check again in the future and remove SetCol_VS SetCol_PS

      src.New("Draw2DCol", "Draw2DCol_VS", "Draw2DCol_PS");
      src.New("Draw3DCol", "Draw3DCol_VS", "Draw3DCol_PS");

      src.New("Draw2DTex" , "Draw2DTex_VS",  "Draw2DTex_PS");
      src.New("Draw2DTexC", "Draw2DTex_VS", "Draw2DTexC_PS");

      src.New("DrawTexX", "Draw2DTex_VS", "DrawTexX_PS");
      src.New("DrawTexY", "Draw2DTex_VS", "DrawTexY_PS");
      src.New("DrawTexZ", "Draw2DTex_VS", "DrawTexZ_PS");
      src.New("DrawTexW", "Draw2DTex_VS", "DrawTexW_PS");

      src.New("DrawTexXG", "Draw2DTex_VS", "DrawTexXG_PS");
      src.New("DrawTexYG", "Draw2DTex_VS", "DrawTexYG_PS");
      src.New("DrawTexZG", "Draw2DTex_VS", "DrawTexZG_PS");
      src.New("DrawTexWG", "Draw2DTex_VS", "DrawTexWG_PS");

      src.New("DrawTexNrm", "Draw2DTex_VS", "DrawTexNrm_PS");
      src.New("Draw"      ,      "Draw_VS",  "Draw2DTex_PS");
      src.New("DrawC"     ,      "Draw_VS", "Draw2DTexC_PS");
      src.New("DrawCG"    ,      "Draw_VS", "DrawTexCG_PS");
      src.New("DrawG"     ,      "Draw_VS", "DrawTexG_PS");
      src.New("DrawA"     ,      "Draw_VS", "Draw2DTexA_PS");

      src.New("DrawX" , "Draw_VS", "DrawX_PS" );
      src.New("DrawXG", "Draw_VS", "DrawXG_PS");
      REPD(dither, 2)
      REPD(gamma , 2)src.New("DrawXC", "Draw_VS", "DrawXC_PS")("DITHER", dither, "GAMMA", gamma);

      src.New("DrawTexPoint" , "Draw2DTex_VS", "DrawTexPoint_PS");
      src.New("DrawTexPointC", "Draw2DTex_VS", "DrawTexPointC_PS");

      src.New("Draw2DTexCol", "Draw2DTexCol_VS", "Draw2DTexCol_PS");

      REPD(alpha_test, 2)
      REPD(color     , 2)
      {
                     src.New("Draw2DDepthTex", "Draw2DDepthTex_VS", "Draw2DDepthTex_PS")("ALPHA_TEST", alpha_test, "COLORS", color);
         REPD(fog, 2)src.New("Draw3DTex"     , "Draw3DTex_VS"     , "Draw3DTex_PS"     )("ALPHA_TEST", alpha_test, "COLORS", color, "FOG", fog);
      }

      if(ms) // Multi-Sampling
      {
         src.New("DrawMs1", "DrawPixel_VS", "DrawMs1_PS");
         src.New("DrawMsN", "DrawPixel_VS", "DrawMsN_PS");
         src.New("DrawMsM", "DrawPixel_VS", "DrawMsM_PS").multiSample();

         src.New("DetectMSCol", "DrawPixel_VS", "DetectMSCol_PS");
       //src.New("DetectMSNrm", "DrawPixel_VS", "DetectMSNrm_PS");
      }

      src.New("DrawMask", "DrawMask_VS", "DrawMask_PS");
      src.New("DrawCubeFace", "DrawCubeFace_VS", "DrawCubeFace_PS");
      src.New("Simple", "Simple_VS", "Simple_PS");

      src.New("Dither", "Draw_VS", "Dither_PS");

                              src.New("CombineSSAlpha", "Draw_VS", "CombineSSAlpha_PS");
      REPD(sample, ms ? 3 : 2)src.New("Combine"       , "Draw_VS", "Combine_PS")("SAMPLE", sample);

      if(ms)src.New("ResolveDepth", "DrawPixel_VS", "ResolveDepth_PS");
      src.New("SetDepth", "Draw_VS", "SetDepth_PS");
      src.New("DrawDepth", "Draw_VS", "DrawDepth_PS");

      REPD(perspective, 2)
      {
                src.New("LinearizeDepth0", "Draw_VS"     , "LinearizeDepth0_PS")("PERSPECTIVE", perspective);
         if(ms){src.New("LinearizeDepth1", "DrawPixel_VS", "LinearizeDepth1_PS")("PERSPECTIVE", perspective);
                src.New("LinearizeDepth2", "DrawPixel_VS", "LinearizeDepth2_PS")("PERSPECTIVE", perspective).multiSample();
         }
      }

      src.New("EdgeDetect"     , "DrawPosXY_VS",      "EdgeDetect_PS");
      src.New("EdgeDetectApply", "Draw_VS"     , "EdgeDetectApply_PS");

      src.New("PaletteDraw", "Draw_VS", "PaletteDraw_PS");

      if(api==API_GL)src.New("WebLToS", "Draw_VS", "WebLToS_PS"); // #WebSRGB

      src.New("Params0", S, "Params0_PS").dummy=true;
      src.New("Params1", S, "Params1_PS").dummy=true;
   }
   { // BLOOM
      ShaderCompiler::Source &src=compiler.New(src_path+"Bloom.cpp");
      REPD(glow    , 2)
      REPD(clamp   , 2)
      REPD(half_res, 2)
      REPD(saturate, 2)
      REPD(gamma   , 2)src.New("BloomDS", "BloomDS_VS", "BloomDS_PS")("GLOW", glow, "CLAMP", clamp, "HALF_RES", half_res, "SATURATE", saturate)("GAMMA", gamma);

      REPD(dither, 2)
      REPD(gamma , 2)src.New("Bloom", "Draw_VS", "Bloom_PS")("DITHER", dither, "GAMMA", gamma);
   }
   { // BLUR
      ShaderCompiler::Source &src=compiler.New(src_path+"Blur.cpp");
      src.New("BlurX", "Draw_VS", "BlurX_PS")("SAMPLES", 4);
      src.New("BlurX", "Draw_VS", "BlurX_PS")("SAMPLES", 6);
      src.New("BlurY", "Draw_VS", "BlurY_PS")("SAMPLES", 4);
      src.New("BlurY", "Draw_VS", "BlurY_PS")("SAMPLES", 6);

      src.New("BlurX_X", "Draw_VS", "BlurX_X_PS");
      src.New("BlurY_X", "Draw_VS", "BlurY_X_PS");

      src.New("MaxX", "Draw_VS", "MaxX_PS");
      src.New("MaxY", "Draw_VS", "MaxY_PS");
   }
   { // CUBIC
      ShaderCompiler::Source &src=compiler.New(src_path+"Cubic.cpp");
      REPD(color, 2)
      {
         src.New("DrawTexCubicFast", "Draw2DTex_VS", "DrawTexCubicFast_PS")("COLORS", color);
         src.New("DrawTexCubic"    , "Draw2DTex_VS", "DrawTexCubic_PS"    )("COLORS", color);
      }
      REPD(dither, 2)
      {
         src.New("DrawTexCubicFastF"   , "Draw_VS", "DrawTexCubicFast_PS"   )("DITHER", dither);
         src.New("DrawTexCubicFastFRGB", "Draw_VS", "DrawTexCubicFastRGB_PS")("DITHER", dither);
         src.New("DrawTexCubicF"       , "Draw_VS", "DrawTexCubic_PS"       )("DITHER", dither);
         src.New("DrawTexCubicFRGB"    , "Draw_VS", "DrawTexCubicRGB_PS"    )("DITHER", dither);
      }
   }
   { // FOG
      ShaderCompiler::Source &src=compiler.New(src_path+"Fog.cpp");
      REPD(multi_sample, ms ? 3 : 1)src.New("Fog", "DrawPosXY_VS", "Fog_PS")("MULTI_SAMPLE", multi_sample).multiSample(multi_sample>=2);
   }
   { // FONT
      ShaderCompiler::Source &src=compiler.New(src_path+"Font.cpp");
      REPD(depth, 2)
      REPD(gamma, 2)
      {
         src.New("Font"  , "Font_VS"  , "Font_PS"  )("SET_DEPTH", depth, "GAMMA", gamma);
         src.New("FontSP", "FontSP_VS", "FontSP_PS")("SET_DEPTH", depth, "GAMMA", gamma);
      }
   }
   { // LIGHT
      ShaderCompiler::Source &src=compiler.New(src_path+"Light.cpp");
      REPD(shadow      , 2)
      REPD(multi_sample, ms ? 2 : 1)
      REPD(quality     , multi_sample ? 1 : 2) // no Quality version for MSAA
      {
                       src.New("LightDir"   , "DrawPosXY_VS", "LightDir_PS"   ).multiSample(multi_sample)("SHADOW", shadow, "MULTI_SAMPLE", multi_sample, "QUALITY", quality);
                       src.New("LightPoint" , "DrawPosXY_VS", "LightPoint_PS" ).multiSample(multi_sample)("SHADOW", shadow, "MULTI_SAMPLE", multi_sample, "QUALITY", quality);
                       src.New("LightLinear", "DrawPosXY_VS", "LightLinear_PS").multiSample(multi_sample)("SHADOW", shadow, "MULTI_SAMPLE", multi_sample, "QUALITY", quality);
         REPD(image, 2)src.New("LightCone"  , "DrawPosXY_VS", "LightCone_PS"  ).multiSample(multi_sample)("SHADOW", shadow, "MULTI_SAMPLE", multi_sample, "QUALITY", quality, "IMAGE", image);
      }
   }
   { // LIGHT APPLY
      ShaderCompiler::Source &src=compiler.New(src_path+"Light Apply.cpp");
      REPD(multi_sample, ms ? 3 : 1)
      REPD(ao          , 2)
      REPD(  cel_shade , 2)
      REPD(night_shade , 2)
         src.New("ApplyLight", "Draw_VS", "ApplyLight_PS")("MULTI_SAMPLE", multi_sample, "AO", ao, "CEL_SHADE", cel_shade, "NIGHT_SHADE", night_shade);
   }
   { // SHADOW
      ShaderCompiler::Source &src=compiler.New(src_path+"Shadow.cpp");
      REPD(multi_sample, ms ? 2 : 1)
      {
         REPD(map_num, 6)
         REPD(cloud  , 2)src.New("ShdDir"  , "DrawPosXY_VS", "ShdDir_PS"  ).multiSample(multi_sample)("MULTI_SAMPLE", multi_sample, "MAP_NUM", map_num+1, "CLOUD", cloud);
                         src.New("ShdPoint", "DrawPosXY_VS", "ShdPoint_PS").multiSample(multi_sample)("MULTI_SAMPLE", multi_sample);
                         src.New("ShdCone" , "DrawPosXY_VS", "ShdCone_PS" ).multiSample(multi_sample)("MULTI_SAMPLE", multi_sample);
      }
      src.New("ShdBlur" , "Draw_VS", "ShdBlur_PS" )("SAMPLES", 4);
    //src.New("ShdBlur" , "Draw_VS", "ShdBlur_PS" )("SAMPLES", 5);
      src.New("ShdBlur" , "Draw_VS", "ShdBlur_PS" )("SAMPLES", 6);
      src.New("ShdBlur" , "Draw_VS", "ShdBlur_PS" )("SAMPLES", 8);
    //src.New("ShdBlur" , "Draw_VS", "ShdBlur_PS" )("SAMPLES", 9);
      src.New("ShdBlur" , "Draw_VS", "ShdBlur_PS" )("SAMPLES", 12);
    //src.New("ShdBlur" , "Draw_VS", "ShdBlur_PS" )("SAMPLES", 13);
    //src.New("ShdBlurX", "Draw_VS", "ShdBlurX_PS")("RANGE", 1);
    //src.New("ShdBlurY", "Draw_VS", "ShdBlurY_PS")("RANGE", 1);
      src.New("ShdBlurX", "Draw_VS", "ShdBlurX_PS")("RANGE", 2);
      src.New("ShdBlurY", "Draw_VS", "ShdBlurY_PS")("RANGE", 2);
   }
   { // OUTLINE
      ShaderCompiler::Source &src=compiler.New(src_path+"Outline.cpp");
      src.New("Outline", "Draw_VS", "Outline_PS")("DOWN_SAMPLE", 0, "CLIP", 0);
      src.New("Outline", "Draw_VS", "Outline_PS")("DOWN_SAMPLE", 1, "CLIP", 0);
      src.New("Outline", "Draw_VS", "Outline_PS")("DOWN_SAMPLE", 0, "CLIP", 1);
      src.New("OutlineApply", "Draw_VS", "OutlineApply_PS");
   }
   { // PARTICLES
      ShaderCompiler::Source &src=compiler.New(src_path+"Particles.cpp");
      REPD(palette             , 2)
      REPD(soft                , 2)
      REPD(anim                , 3)
      REPD(motion_affects_alpha, 2)
         src.New("Particle", "Particle_VS", "Particle_PS")("PALETTE", palette, "SOFT", soft, "ANIM", anim)("MOTION_STRETCH", 1, "MOTION_AFFECTS_ALPHA", motion_affects_alpha);
         src.New("Particle", "Particle_VS", "Particle_PS")("PALETTE",       0, "SOFT",    0, "ANIM",    0)("MOTION_STRETCH", 0, "MOTION_AFFECTS_ALPHA",                    0);
   }
   { // SKY
      ShaderCompiler::Source &src=compiler.New(src_path+"Sky.cpp");
      REPD(dither, 2)
      REPD(cloud , 2)
      {
         // Textures Flat
         REPD(textures, 2)src.New("Sky", "Sky_VS", "Sky_PS")("MULTI_SAMPLE", 0, "FLAT", 1, "DENSITY", 0, "TEXTURES", textures+1)("STARS", 0, "DITHER", dither, "PER_VERTEX", 0, "CLOUD", cloud);

         // Atmospheric Flat
         REPD(per_vertex, 2)
         REPD(stars     , 2)src.New("Sky", "Sky_VS", "Sky_PS")("MULTI_SAMPLE", 0, "FLAT", 1, "DENSITY", 0, "TEXTURES", 0)("STARS", stars, "DITHER", dither, "PER_VERTEX", per_vertex, "CLOUD", cloud);

         REPD(multi_sample, ms ? 3 : 1)
         {
            // Textures
            REPD(textures, 2)src.New("Sky", "Sky_VS", "Sky_PS")("MULTI_SAMPLE", multi_sample, "FLAT", 0, "DENSITY", 0, "TEXTURES", textures+1)("STARS", 0, "DITHER", dither, "PER_VERTEX", 0, "CLOUD", cloud).multiSample(multi_sample>=2);

            // Atmospheric
            REPD(per_vertex, 2)
            REPD(density   , 2)
            REPD(stars     , 2)src.New("Sky", "Sky_VS", "Sky_PS")("MULTI_SAMPLE", multi_sample, "FLAT", 0, "DENSITY", density, "TEXTURES", 0)("STARS", stars, "DITHER", dither, "PER_VERTEX", per_vertex, "CLOUD", cloud).multiSample(multi_sample>=2);
         }
      }
   }
   { // SMAA
      ShaderCompiler::Source &src=compiler.New(src_path+"SMAA.cpp");
      REPD(gamma, 2)src.New("SMAAEdge" , "SMAAEdge_VS" , "SMAAEdge_PS" )("GAMMA", gamma);
                    src.New("SMAABlend", "SMAABlend_VS", "SMAABlend_PS");
                    src.New("SMAA"     , "SMAA_VS"     , "SMAA_PS"     );
                 #if SUPPORT_MLAA
                    src.New("MLAAEdge" , "MLAA_VS", "MLAAEdge_PS" );
                    src.New("MLAABlend", "MLAA_VS", "MLAABlend_PS");
                    src.New("MLAA"     , "MLAA_VS", "MLAA_PS"     );
                 #endif
   }
   { // SUN
      ShaderCompiler::Source &src=compiler.New(src_path+"Sun.cpp");
      REPD(mask, 2)src.New("SunRaysMask", "DrawPosXY_VS", "SunRaysMask_PS")("MASK", mask);

      REPD(mask  , 2)
      REPD(dither, 2)
      REPD(jitter, 2)
      REPD(gamma , 2)
         src.New("SunRays", "DrawPosXY_VS", "SunRays_PS")("MASK", mask, "DITHER", dither, "JITTER", jitter, "GAMMA", gamma);
   }
   { // VIDEO
      ShaderCompiler::Source &src=compiler.New(src_path+"Video.cpp");
      REPD(gamma, 2)
      REPD(alpha, 2)
         src.New("YUV", "Draw2DTex_VS", "YUV_PS")("GAMMA", gamma, "ALPHA", alpha);
   }
}
#endif

#ifdef AMBIENT
{
   Str names;

   // base
   REPD(skin      , 2)
   REPD(alpha_test, 3)
   REPD(light_map , 2)
      names+=TechAmbient(skin, alpha_test, light_map);

   Add(src_path+"Ambient.cpp", dest_path+"Ambient", api, model, names);
}
#endif

#ifdef AMBIENT_OCCLUSION
{
   ShaderCompiler::Source &src=ShaderCompilers.New().set(dest_path+"Ambient Occlusion", model, api).New(src_path+"Ambient Occlusion.cpp");
   REPD(mode  , 4)
   REPD(jitter, 2)
   REPD(normal, 2)
      src.New("AO", "AO_VS", "AO_PS")("MODE", mode, "JITTER", jitter, "NORMALS", normal);
}
#endif

#ifdef BEHIND
{
   Str names;

   // base
   REPD(skin    , 2)
   REPD(textures, 3)names+=TechBehind(skin, textures);

   Add(src_path+"Behind.cpp", dest_path+"Behind", api, model, names);
}
#endif

#ifdef BLEND
{
   Str names; Memc<ShaderGLSL> glsl;

   REPD(skin    , 2)
   REPD(color   , 2)
   REPD(rflct   , 2)
   REPD(textures, 3)
   {
       names+=(TechBlend    (skin, color, rflct, textures));
      glsl.add(TechBlendGlsl(skin, color, rflct, textures));
   }

   Add(src_path+"Blend.cpp", dest_path+"Blend", api, model, names, glsl);
}
#endif

#ifdef BONE_HIGHLIGHT
{
   ShaderCompiler::Source &src=ShaderCompilers.New().set(dest_path+"Bone Highlight", model, api).New(src_path+"Bone Highlight.cpp");
   REPD(skin     , 2)
   REPD(bump_mode, 2)src.New(S, "VS", "PS")("SKIN", skin, "BUMP_MODE", bump_mode ? SBUMP_FLAT : SBUMP_ZERO);
}
#endif

#ifdef DEPTH_OF_FIELD
   Add(src_path+"Depth of Field.cpp", dest_path+"Depth of Field", api, model);
#endif

#ifdef EARLY_Z
{
   ShaderCompiler::Source &src=ShaderCompilers.New().set(dest_path+"Early Z", model, api).New(src_path+"Early Z.cpp");
   REPD(skin, 2)src.New(S, "VS", "PS")("SKIN", skin);
}
#endif

#ifdef EFFECTS_2D
{
   ShaderCompiler::Source &src=ShaderCompilers.New().set(dest_path+"Effects 2D", model, api).New(src_path+"Effects 2D.cpp");
   src.New("ColTrans"   , "Draw_VS", "ColTrans_PS");
   src.New("ColTransHB" , "Draw_VS", "ColTransHB_PS");
   src.New("ColTransHSB", "Draw_VS", "ColTransHSB_PS");

   src.New("Fade", "Draw_VS", "Fade_PS");
   src.New("RadialBlur", "Draw_VS", "RadialBlur_PS");
   src.New("Ripple", "Draw2DTex_VS", "Ripple_PS");
   src.New("Wave", "Wave_VS", "Wave_PS");
}
#endif

#ifdef EFFECTS_3D
   Add(src_path+"Effects 3D.cpp", dest_path+"Effects 3D", api, model);
#endif

#ifdef FOG_LOCAL
{
   ShaderCompiler::Source &src=ShaderCompilers.New().set(dest_path+"Fog Local", model, api).New(src_path+"Fog.cpp");
   REPD(height, 2)
   {
                     src.New("FogBox"  , "FogBox_VS"  , "FogBox_PS"  )("HEIGHT", height);
      REPD(inside, 2)src.New("FogBoxI" , "FogBoxI_VS" , "FogBoxI_PS" )("HEIGHT", height)("INSIDE", inside);
   }
                     src.New("FogBall" , "FogBall_VS" , "FogBall_PS" );
      REPD(inside, 2)src.New("FogBallI", "FogBallI_VS", "FogBallI_PS")("INSIDE", inside);
}
#endif

#ifdef FUR
{
   Str names;

   // base
   REPD(skin   , 2)
   REPD(size   , 2)
   REPD(diffuse, 2)names+=TechFurBase(skin, size, diffuse);

   // soft
   REPD(skin   , 2)
   REPD(size   , 2)
   REPD(diffuse, 2)names+=TechFurSoft(skin, size, diffuse);

   Add(src_path+"Fur.cpp", dest_path+"Fur", api, model, names);
}
#endif

#ifdef FXAA // FXAA unlike SMAA is kept outside of Main shader, because it's rarely used.
{
   ShaderCompiler::Source &src=ShaderCompilers.New().set(dest_path+"FXAA", model, api).New(src_path+"FXAA.cpp");
   REPD(gamma, 2)src.New("FXAA", "Draw_VS", "FXAA_PS")("GAMMA", gamma);
}
#endif

#ifdef HDR
   Add(src_path+"Hdr.cpp", dest_path+"Hdr", api, model);
#endif

#ifdef LAYERED_CLOUDS
   Add(src_path+"Layered Clouds.cpp", dest_path+"Layered Clouds", api, model);
#endif

#ifdef MOTION_BLUR
   Add(src_path+"Motion Blur.cpp", dest_path+"Motion Blur", api, model);
#endif

#ifdef OVERLAY
{
   Str names;

   // base
   REPD(skin  , 2)
   REPD(normal, 2)names+=TechOverlay(skin, normal);

   Add(src_path+"Overlay.cpp", dest_path+"Overlay", api, model, names);
}
{
   Str names;

   // base
   REPD(tess, (model>=SM_4) ? 2 : 1)
   REPD(skin, 2)names+=TechTattoo(skin, tess);

   Add(src_path+"Tattoo.cpp", dest_path+"Tattoo", api, model, names);
}
#endif

#ifdef POSITION
{
   ShaderCompiler::Source &src=ShaderCompilers.New().set(dest_path+"Position", model, api).New(src_path+"Position.cpp");
   REPD(tesselate , tess ? 2 : 1)
   REPD(skin      , 2)
   REPD(textures  , 3)
   REPD(test_blend, textures ? 2 : 1)src.New().position(skin, textures, test_blend, FX_NONE, tesselate);

   // grass + leafs
   for(Int textures=1; textures<=2; textures++)
   REPD(test_blend, 2)
   {
      src.New().position(0, textures, test_blend, FX_GRASS, 0);
      src.New().position(0, textures, test_blend, FX_LEAF , 0);
      src.New().position(0, textures, test_blend, FX_LEAFS, 0);
   }
}
#endif

#ifdef SET_COLOR
{
   Str names;

   // base
   REPD(tess    , (model>=SM_4) ? 2 : 1)
   REPD(skin    , 2)
   REPD(textures, 3)names+=TechSetColor(skin, textures, tess);

   Add(src_path+"Set Color.cpp", dest_path+"Set Color", api, model, names);
}
#endif

#ifdef VOLUMETRIC_CLOUDS
   Add(src_path+"Volumetric Clouds.cpp", dest_path+"Volumetric Clouds", api, model);
#endif

#ifdef VOLUMETRIC_LIGHTS
   Add(src_path+"Volumetric Lights.cpp", dest_path+"Volumetric Lights", api, model);
#endif

#ifdef WATER
   Add(src_path+"Water.cpp", dest_path+"Water", api, model);
#endif

#ifdef WORLD_EDITOR
   Add(src_path+"World Editor.cpp", dest_path+"World Editor", api, model);
#endif

#ifdef DX10_INPUT_LAYOUT
   Add(src_path+"DX10+ Input Layout.cpp", S, api, model);
#endif

#ifdef DEFERRED
{
   Str names;

   // zero
   REPD(skin , 2)
   REPD(color, 2)names+=TechDeferred(skin, 1, 0, SBUMP_ZERO, false, false, false, false, color, false, false, FX_NONE, false);

   REPD(tess     , (model>=SM_4) ? 2 : 1)
   REPD(heightmap, 2)
   {
      // 1 material, 0-2 tex, flat
      REPD(skin  , heightmap ? 1 : 2)
      REPD(detail, 2)
      REPD(rflct , 2)
      REPD(color , 2)
      {
         names+=TechDeferred(skin, 1, 0, SBUMP_FLAT, false, detail, false, rflct, color, false, heightmap, FX_NONE, tess); // 1 material, 0 tex
         REPD(alpha_test, heightmap ? 1 : 2)
         {
            names+=TechDeferred(skin, 1, 1, SBUMP_FLAT, alpha_test, detail, false, rflct, color, false, heightmap, FX_NONE, tess); // 1 material, 1 tex
            names+=TechDeferred(skin, 1, 2, SBUMP_FLAT, alpha_test, detail, false, rflct, color, false, heightmap, FX_NONE, tess); // 1 material, 2 tex
         }
      }

      // 1 material, 1-2 tex, flat, macro
      REPD(color, 2)
      for(Int textures=1; textures<=2; textures++)names+=TechDeferred(false, 1, textures, SBUMP_FLAT, false, false, true, false, color, false, heightmap, FX_NONE, tess);

   #if BUMP_MAPPING
      // 1 material, 2 tex, normal + parallax
      REPD(skin      , heightmap ? 1 : 2)
      REPD(alpha_test, heightmap ? 1 : 2)
      REPD(detail    , 2)
      REPD(rflct     , 2)
      REPD(color     , 2)
      for(Int bump_mode=SBUMP_NORMAL; bump_mode<=SBUMP_PARALLAX_MAX; bump_mode++)if(bump_mode==SBUMP_NORMAL || bump_mode>=SBUMP_PARALLAX_MIN)
         names+=TechDeferred(skin, 1, 2, bump_mode, alpha_test, detail, false, rflct, color, false, heightmap, FX_NONE, tess);

      // 1 material, 1-2 tex, normal, macro
      REPD(color, 2)
      for(Int textures=1; textures<=2; textures++)names+=TechDeferred(false, 1, textures, SBUMP_NORMAL, false, false, true, false, color, false, heightmap, FX_NONE, tess);

      // 1 material, 2 tex, relief
      REPD(skin      , heightmap ? 1 : 2)
      REPD(alpha_test, heightmap ? 1 : 2)
      REPD(detail    , 2)
      REPD(rflct     , 2)
      REPD(color     , 2)names+=TechDeferred(skin, 1, 2, SBUMP_RELIEF, alpha_test, detail, false, rflct, color, false, heightmap, FX_NONE, tess);
   #endif

   #if MULTI_MATERIAL
      for(Int materials=2; materials<=MAX_MTRLS; materials++)
      REPD(color     , 2)
      REPD(mtrl_blend, 2)
      REPD(rflct     , 2)
      {
         // 2-4 materials, 1-2 tex, flat
         REPD(detail, 2)
         for(Int textures=1; textures<=2; textures++)names+=TechDeferred(false, materials, textures, SBUMP_FLAT, false, detail, false, rflct, color, mtrl_blend, heightmap, FX_NONE, tess);

         // 2-4 materials, 1-2 tex, flat, macro
         for(Int textures=1; textures<=2; textures++)names+=TechDeferred(false, materials, textures, SBUMP_FLAT, false, false, true, rflct, color, mtrl_blend, heightmap, FX_NONE, tess);

      #if BUMP_MAPPING
         // 2-4 materials, 2 textures, normal + parallax
         REPD(detail, 2)
         for(Int bump_mode=SBUMP_NORMAL; bump_mode<=SBUMP_PARALLAX_MAX_MULTI; bump_mode++)if(bump_mode==SBUMP_NORMAL || bump_mode>=SBUMP_PARALLAX_MIN)
            names+=TechDeferred(false, materials, 2, bump_mode, false, detail, false, rflct, color, mtrl_blend, heightmap, FX_NONE, tess);

         // 2-4 materials, 2 textures, normal, macro
         names+=TechDeferred(false, materials, 2, SBUMP_NORMAL, false, false, true, rflct, color, mtrl_blend, heightmap, FX_NONE, tess);

         // 2-4 materials, 2 textures, relief
         REPD(detail, 2)
            names+=TechDeferred(false, materials, 2, SBUMP_RELIEF, false, detail, false, rflct, color, mtrl_blend, heightmap, FX_NONE, tess);
      #endif
      }
   #endif
   }

   // grass + leaf
   for(Int textures=1; textures<=2; textures++)
   REPD(color, 2)
   {
      names+=TechDeferred(false, 1, textures, SBUMP_FLAT, true, false, false, false, color, false, false, FX_GRASS, false); // 1 material, 1-2 tex, grass, flat
      names+=TechDeferred(false, 1, textures, SBUMP_FLAT, true, false, false, false, color, false, false, FX_LEAF , false); // 1 material, 1-2 tex, leaf , flat
      names+=TechDeferred(false, 1, textures, SBUMP_FLAT, true, false, false, false, color, false, false, FX_LEAFS, false); // 1 material, 1-2 tex, leafs, flat
      if(textures==2)
      {
         names+=TechDeferred(false, 1, textures, SBUMP_NORMAL, true, false, false, false, color, false, false, FX_GRASS, false); // 1 material, 1-2 tex, grass, normal
         names+=TechDeferred(false, 1, textures, SBUMP_NORMAL, true, false, false, false, color, false, false, FX_LEAF , false); // 1 material, 1-2 tex, leaf , normal
         names+=TechDeferred(false, 1, textures, SBUMP_NORMAL, true, false, false, false, color, false, false, FX_LEAFS, false); // 1 material, 1-2 tex, leafs, normal
      }
   }

   Add(src_path+"Deferred.cpp", dest_path+"Deferred", api, model, names);
}
#endif

#ifdef BLEND_LIGHT
{
   Str names; Memc<ShaderGLSL> glsl;

   REPD(per_pixel  , 2)
   REPD(shadow_maps, per_pixel ? 7 : 1) // 7=(6+off), 1=off
   {
      // base
      REPD(skin      , 2)
      REPD(color     , 2)
      REPD(textures  , 3)
      REPD(bump_mode , (per_pixel && textures==2) ? 2 : 1)
      REPD(alpha_test,               textures     ? 2 : 1)
      REPD(alpha     ,               textures     ? 2 : 1)
      REPD(light_map ,               textures     ? 2 : 1)
      REPD(rflct     , 2)
      {
                                            names+=(TechBlendLight    (skin, color, textures, bump_mode ? SBUMP_NORMAL: SBUMP_FLAT, alpha_test, alpha, light_map, rflct, FX_NONE, per_pixel, shadow_maps));
         if(shadow_maps==0 && bump_mode==0)glsl.add(TechBlendLightGlsl(skin, color, textures, bump_mode ? SBUMP_NORMAL: SBUMP_FLAT, alpha_test, alpha, light_map, rflct, FX_NONE, per_pixel, shadow_maps));
      }

      // grass+leaf
      REPD(color     , 2)
      REPD(textures  , 3)
      REPD(bump_mode , (per_pixel && textures==2) ? 2 : 1)
      REPD(alpha_test,               textures     ? 2 : 1)
      {
                                            names+=(TechBlendLight    (false, color, textures, bump_mode ? SBUMP_NORMAL : SBUMP_FLAT, alpha_test, true, false, false, FX_GRASS, per_pixel, shadow_maps));
                                            names+=(TechBlendLight    (false, color, textures, bump_mode ? SBUMP_NORMAL : SBUMP_FLAT, alpha_test, true, false, false, FX_LEAF , per_pixel, shadow_maps));
                                            names+=(TechBlendLight    (false, color, textures, bump_mode ? SBUMP_NORMAL : SBUMP_FLAT, alpha_test, true, false, false, FX_LEAFS, per_pixel, shadow_maps));
         if(shadow_maps==0 && bump_mode==0)glsl.add(TechBlendLightGlsl(false, color, textures, bump_mode ? SBUMP_NORMAL : SBUMP_FLAT, alpha_test, true, false, false, FX_GRASS, per_pixel, shadow_maps));
         if(shadow_maps==0 && bump_mode==0)glsl.add(TechBlendLightGlsl(false, color, textures, bump_mode ? SBUMP_NORMAL : SBUMP_FLAT, alpha_test, true, false, false, FX_LEAF , per_pixel, shadow_maps));
         if(shadow_maps==0 && bump_mode==0)glsl.add(TechBlendLightGlsl(false, color, textures, bump_mode ? SBUMP_NORMAL : SBUMP_FLAT, alpha_test, true, false, false, FX_LEAFS, per_pixel, shadow_maps));
      }
   }

   Add(src_path+"Blend Light.cpp", dest_path+"Blend Light", api, model, names, glsl);
}
#endif

#ifdef SIMPLE
{
   Str names; Memc<ShaderGLSL> glsl;

   REPD(per_pixel, 2)
   {
      // zero
      REPD(skin , 2)
      REPD(color, 2)
      {
          names+=(TechSimple    (skin, 1, 0, SBUMP_ZERO, false, false, false, color, false, false, FX_NONE, per_pixel, false));
         glsl.add(TechSimpleGlsl(skin, 1, 0, SBUMP_ZERO, false, false, false, color, false, false, FX_NONE, per_pixel, false));
      }

      REPD(tess     , (model>=SM_4) ? 2 : 1)
      REPD(heightmap, 2)
      {
         // 1 material, 0-2 textures
         REPD(skin    , heightmap ? 1 : 2)
         REPD(rflct   , 2)
         REPD(color   , 2)
       //REPD(instance, (model>=SM_4 && !skin) ? 2 : 1)
         {
             names+=(TechSimple    (skin, 1, 0, SBUMP_FLAT, false, false, rflct, color, false, heightmap, FX_NONE, per_pixel, tess)); // 1 material, 0 tex
            glsl.add(TechSimpleGlsl(skin, 1, 0, SBUMP_FLAT, false, false, rflct, color, false, heightmap, FX_NONE, per_pixel, tess));
            REPD(alpha_test, heightmap ? 1 : 2)
            REPD(light_map , 2)
            {
                names+=(TechSimple    (skin, 1, 1, SBUMP_FLAT, alpha_test, light_map, rflct, color, false, heightmap, FX_NONE, per_pixel, tess)); // 1 material, 1 tex
                names+=(TechSimple    (skin, 1, 2, SBUMP_FLAT, alpha_test, light_map, rflct, color, false, heightmap, FX_NONE, per_pixel, tess)); // 1 material, 2 tex
               glsl.add(TechSimpleGlsl(skin, 1, 1, SBUMP_FLAT, alpha_test, light_map, rflct, color, false, heightmap, FX_NONE, per_pixel, tess));
               glsl.add(TechSimpleGlsl(skin, 1, 2, SBUMP_FLAT, alpha_test, light_map, rflct, color, false, heightmap, FX_NONE, per_pixel, tess));
            }
         }

      #if MULTI_MATERIAL
         // 2-4 materials, 1 textures
         for(Int materials=2; materials<=MAX_MTRLS; materials++)
         REPD(color     , 2)
         REPD(mtrl_blend, 2)
         REPD(rflct     , 2)
         {
                       names+=(TechSimple    (false, materials, 1, SBUMP_FLAT, false, false, rflct, color, mtrl_blend, heightmap, FX_NONE, per_pixel, tess));
            if(!rflct)glsl.add(TechSimpleGlsl(false, materials, 1, SBUMP_FLAT, false, false, rflct, color, mtrl_blend, heightmap, FX_NONE, per_pixel, tess));
         }
      #endif
      }

      // grass + leaf
    //REPD(instance, (model>=SM_4) ? 2 : 1)
      REPD(color, 2)
      for(Int textures=1; textures<=2; textures++)
      {
          names+=(TechSimple    (false, 1, textures, SBUMP_FLAT, true, false, false, color, false, false, FX_GRASS, per_pixel, false));
          names+=(TechSimple    (false, 1, textures, SBUMP_FLAT, true, false, false, color, false, false, FX_LEAF , per_pixel, false));
          names+=(TechSimple    (false, 1, textures, SBUMP_FLAT, true, false, false, color, false, false, FX_LEAFS, per_pixel, false));
         glsl.add(TechSimpleGlsl(false, 1, textures, SBUMP_FLAT, true, false, false, color, false, false, FX_GRASS, per_pixel, false));
         glsl.add(TechSimpleGlsl(false, 1, textures, SBUMP_FLAT, true, false, false, color, false, false, FX_LEAF , per_pixel, false));
         glsl.add(TechSimpleGlsl(false, 1, textures, SBUMP_FLAT, true, false, false, color, false, false, FX_LEAFS, per_pixel, false));
      }
   }

   // bone highlight
   REPD(skin, 2)names+=TechSimple(skin, 1, 0, SBUMP_FLAT, false, false, false, false, false, false, FX_BONE, true, false); // !! this name must be in sync with other calls in the engine that use FX_BONE !!

   Add(src_path+"Simple.cpp", dest_path+"Simple", api, model, names, glsl);
}
#endif

#ifdef FORWARD
{
   Str names;

   // zero
   REPD(skin , 2)
   REPD(color, 2)names+=TechForward(skin, 1, 0, SBUMP_ZERO, false, false, false, false, color, false, false, FX_NONE,   false,false,0,   false,false,   false,false,   false,false,   false);

   REPD(tess     , (model>=SM_4) ? 2 : 1)
   REPD(heightmap, 2)
   {
      // 1 material, 0-2 textures, flat
      REPD(skin  , heightmap ? 1 : 2)
      REPD(detail, 2)
      REPD(rflct , 2)
      REPD(color , 2)
      {
         names+=TechForwardLight(skin, 1, 0, SBUMP_FLAT, false, false, detail, rflct, color, false, heightmap, FX_NONE, tess); // 1 material, 0 tex
         REPD(alpha_test, heightmap ? 1 : 2)
         REPD(light_map , 2)
         {
            names+=TechForwardLight(skin, 1, 1, SBUMP_FLAT, alpha_test, light_map, detail, rflct, color, false, heightmap, FX_NONE, tess); // 1 material, 1 tex
            names+=TechForwardLight(skin, 1, 2, SBUMP_FLAT, alpha_test, light_map, detail, rflct, color, false, heightmap, FX_NONE, tess); // 1 material, 2 tex
         }
      }

   #if BUMP_MAPPING
      // 1 material, 2 tex, normal
      REPD(skin      , heightmap ? 1 : 2)
      REPD(alpha_test, heightmap ? 1 : 2)
      REPD(light_map , 2)
      REPD(detail    , 2)
      REPD(rflct     , 2)
      REPD(color     , 2)names+=TechForwardLight(skin, 1, 2, SBUMP_NORMAL, alpha_test, light_map, detail, rflct, color, false, heightmap, FX_NONE, tess);
   #endif

   #if MULTI_MATERIAL
      for(Int materials=2; materials<=MAX_MTRLS; materials++)
      REPD(color     , 2)
      REPD(mtrl_blend, 2)
      REPD(rflct     , 2)
      {
         // 2-4 materials, 1-2 textures, flat
         for(Int textures=1; textures<=2; textures ++)names+=TechForwardLight(false, materials, textures, SBUMP_FLAT, false, false, false, rflct, color, mtrl_blend, heightmap, FX_NONE, tess);

      #if BUMP_MAPPING
         // 2-4 materials, 2 textures, normal
         names+=TechForwardLight(false, materials, 2, SBUMP_NORMAL, false, false, false, rflct, color, mtrl_blend, heightmap, FX_NONE, tess);
      #endif
      }
   #endif
   }

   // grass + leaf
   for(Int textures=1; textures<=2; textures++)
   REPD(color, 2)
   {
      names+=TechForwardLight(false, 1, textures, SBUMP_FLAT, true, false, false, false, color, false, false, FX_GRASS, false); // 1 material, 1-2 tex, grass, flat
      names+=TechForwardLight(false, 1, textures, SBUMP_FLAT, true, false, false, false, color, false, false, FX_LEAF , false); // 1 material, 1-2 tex, leaf , flat
      names+=TechForwardLight(false, 1, textures, SBUMP_FLAT, true, false, false, false, color, false, false, FX_LEAFS, false); // 1 material, 1-2 tex, leafs, flat
      if(textures==2)
      {
         names+=TechForwardLight(false, 1, textures, SBUMP_NORMAL, true, false, false, false, color, false, false, FX_GRASS, false); // 1 material, 1-2 tex, grass, normal
         names+=TechForwardLight(false, 1, textures, SBUMP_NORMAL, true, false, false, false, color, false, false, FX_LEAF , false); // 1 material, 1-2 tex, leaf , normal
         names+=TechForwardLight(false, 1, textures, SBUMP_NORMAL, true, false, false, false, color, false, false, FX_LEAFS, false); // 1 material, 1-2 tex, leafs, normal
      }
   }

   Add(src_path+"Forward.cpp", dest_path+"Forward", api, model, names);
}
#endif
}
/******************************************************************************/
#endif // COMPILE
/******************************************************************************/
void MainShaderClass::compile()
{
#if COMPILE_4 || COMPILE_GL
   App.stayAwake(AWAKE_SYSTEM);

#if COMPILE_GL
   Compile(API_GL);
#endif
#if COMPILE_4
   Compile(API_DX);
#endif

   ProcPriority(-1); // compiling shaders may slow down entire CPU, so make this process have smaller priority
   Dbl t=Time.curTime();
   //FIXME MultiThreadedCall(OldShaderCompilers, ThreadCompile);
   if(ShaderCompilers.elms())
   {
      Threads threads; threads.create(false, Cpu.threads()-1);
      FREPAO(ShaderCompilers).compile(threads);
   }

   App.stayAwake(AWAKE_OFF);
#endif
}
/******************************************************************************/
}
/******************************************************************************/
