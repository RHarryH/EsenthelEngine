/******************************************************************************/
struct ShaderCompiler
{
   enum API : Byte
   {
      API_DX,
      API_GL,
      API_VULKAN,
      API_METAL,
      API_NUM,
   };
   static inline CChar8* APIName[]=
   {
      "DX",
      "GL",
      "VULKAN",
      "METAL",
   };

   enum SHADER_TYPE : Byte
   {
      VS,
      HS,
      DS,
      PS,
      ST_NUM,
   };

   struct Param
   {
      Str8 name;
      Int  elms, cpu_data_size=0, gpu_data_size;
      Mems<ShaderParam::Translation> translation;
      Mems<Byte> data;

      Bool operator==(C Param &p)C;
      Bool operator!=(C Param &p)C {return !(T==p);}

      void addTranslation(ID3D11ShaderReflectionType *type, C D3D11_SHADER_TYPE_DESC &type_desc, CChar8 *name, Int &offset, SByte &was_min16); // 'was_min16'=if last inserted parameter was of min16 type (-1=no last parameter)
   };
   struct Buffer
   {
      Str8 name;
      Int  size, bind_slot;
      Bool bind_explicit;
    //Mems<Byte>  data;
      Mems<Param> params;

      Bool operator==(C Buffer &b)C;
      Bool operator!=(C Buffer &b)C {return !(T==b);}
   };
   struct Image
   {
      Str8 name;
      Int  bind_slot;
   };

   struct IO
   {
      Str8 name;
      Int  index, reg;

      void operator=(C D3D11_SIGNATURE_PARAMETER_DESC &desc) {name=desc.SemanticName; index=desc.SemanticIndex; reg=desc.Register;}
      Bool operator==(C IO &io)C {return name==io.name && index==io.index && reg==io.reg;}
      Bool operator!=(C IO &io)C {return !(T==io);}
   };

   enum RESULT : Byte
   {
      NONE,
      FAIL,
      GOOD,
   };

   struct Shader;
   struct SubShader
   {
    C Shader      *shader;
      SHADER_TYPE  type;
      RESULT       result=NONE;
      Str8         func_name;
      Str          error;
      Mems<Buffer> buffers;
      Mems<Image > images;
      Mems<IO    > inputs, outputs;
      Mems<Byte  > shader_data;
      Int          shader_data_index;

      Bool is()C {return func_name.is();}
      void compile();
   };

   struct TextParam8
   {
      Str8 name, value;
      void set(C Str8 &name, C Str8 &value) {T.name=name; T.value=value;}
   };

   struct Source;
   struct Shader
   {
      Str              name;
      SHADER_MODEL     model;
      Memc<TextParam8> params;
      SubShader        sub[ST_NUM];
    C Source          *source;

      Shader& Model(SHADER_MODEL model) {T.model=model; return T;} // override model (needed for tesselation)

      Shader& operator()(C Str &n0, C Str &v0                                                                     ) {params.New().set(n0, v0);                                                                               return T;}
      Shader& operator()(C Str &n0, C Str &v0,  C Str &n1, C Str &v1                                              ) {params.New().set(n0, v0); params.New().set(n1, v1);                                                     return T;}
      Shader& operator()(C Str &n0, C Str &v0,  C Str &n1, C Str &v1,  C Str &n2, C Str &v2                       ) {params.New().set(n0, v0); params.New().set(n1, v1); params.New().set(n2, v2);                           return T;}
      Shader& operator()(C Str &n0, C Str &v0,  C Str &n1, C Str &v1,  C Str &n2, C Str &v2,  C Str &n3, C Str &v3) {params.New().set(n0, v0); params.New().set(n1, v1); params.New().set(n2, v2); params.New().set(n3, v3); return T;}

      void finalizeName();
   };

   struct Source
   {
      Str             file_name;
      Mems<Byte>      file_data;
      Memc<Shader>    shaders;
      SHADER_MODEL    model;
      ShaderCompiler *compiler;

      Shader& New(C Str &name, C Str8 &vs, C Str8 &ps);
      Bool    load();
   };
   Str          dest, messages;
   SHADER_MODEL model;
   API          api;
   Memc<Source> sources;

   void message(C Str &t) {messages.line()+=t;}
   Bool error  (C Str &t) {message(t); return false;}

   ShaderCompiler& set(C Str &dest, SHADER_MODEL model, API api=API_DX);
   Source& New(C Str &file_name);
   /*Bool save()C
   {
      File f; if(f.writeTry(dest))
      {
         f.putUInt (CC4_SHDR   ); // cc4
         f.putByte (SHADER_DX11); // type
         f.cmpUIntV(0          ); // version

         // constants
         f.cmpUIntV(buffers.elms()); FREPA(buffers)
         {
          C ShaderBufferParams &buf=buffers[i];

            // constant buffer
            f.putStr(Name(*buf.buffer)).cmpUIntV(buf.buffer->size()).putSByte(buf.index); DYNAMIC_ASSERT(buf.index>=-1 && buf.index<=127, "buf.index out of range");

            // params
            if(!buf.params.save(f))return false;
         }

         // images
         f.cmpUIntV(images.elms());
         FREPA(images)f.putStr(Name(*images[i]));

         if(vs   .save(f)) // shaders
         if(hs   .save(f))
         if(ds   .save(f))
         if(ps   .save(f))
      // FIXME don't list constant buffers that have 'bind_explicit' in vs_buffers, etc, but LIST in buffers
         if(techs.save(f, buffers, images)) // techniques
            if(f.flushOK())return true;

         f.del(); FDelFile(name);
      }
      return false;
   }*/
   Bool compileTry(Threads &threads);
   void compile   (Threads &threads);
};
/******************************************************************************/
