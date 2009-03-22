#include "OpenGLShader.h"
#include "GLProgramFactory.h"
#include "OpenGLStateBucketAdd.h"
#include "render/OpenGLShaderCache.h"

#include "iuimanager.h"
#include "ishaders.h"
#include "ifilter.h"
#include "irender.h"
#include "generic/callback.h"
#include "texturelib.h"

void OpenGLShader::destroy() {
	// Clear the shaderptr, so that the shared_ptr reference count is decreased 
    _iShader = IShaderPtr();

    for(Passes::iterator i = _shaderPasses.begin(); i != _shaderPasses.end(); ++i)
    {
      delete *i;
    }
    _shaderPasses.clear();
}

void OpenGLShader::addRenderable(const OpenGLRenderable& renderable, 
					   			 const Matrix4& modelview, 
					   			 const LightList* lights)
{
	// Iterate over the list of OpenGLStateBuckets, bumpmap and non-bumpmap
	// buckets are handled differently.
    for(Passes::iterator i = _shaderPasses.begin(); i != _shaderPasses.end(); ++i)
    {
      if(((*i)->state().renderFlags & RENDER_BUMP) != 0)
      {
        if(lights != 0)
        {
          OpenGLStateBucketAdd add(*(*i), renderable, modelview);
          lights->forEachLight(makeCallback1(add));
        }
      }
      else
      {
        (*i)->addRenderable(renderable, modelview);
      }
    }
}

void OpenGLShader::incrementUsed() {
    if(++m_used == 1 && _iShader != 0)
    { 
      _iShader->SetInUse(true);
    }
}

void OpenGLShader::decrementUsed() {
    if(--m_used == 0 && _iShader != 0)
    {
      _iShader->SetInUse(false);
    }
}

void OpenGLShader::realise(const std::string& name) 
{
    // Construct the shader passes based on the name
    construct(name);

    if (_iShader != NULL) {
		// greebo: Check the filtersystem whether we're filtered
		_iShader->setVisible(GlobalFilterSystem().isVisible("texture", name));

		if (m_used != 0) {
			_iShader->SetInUse(true);
		}
    }
    
    for(Passes::iterator i = _shaderPasses.begin(); i != _shaderPasses.end(); ++i) {
    	render::getOpenGLShaderCache().insertSortedState(
			OpenGLStates::value_type(OpenGLStateReference((*i)->state()), 
									 *i));
    }

    m_observers.realise();
}

void OpenGLShader::unrealise() {
    m_observers.unrealise();

    for(Passes::iterator i = _shaderPasses.begin(); i != _shaderPasses.end(); ++i) {
    	render::getOpenGLShaderCache().eraseSortedState(
    		OpenGLStateReference((*i)->state())
    	);
    }

    destroy();
}

unsigned int OpenGLShader::getFlags() const {
    return _iShader->getFlags();
}

// Append a default shader pass onto the back of the state list
OpenGLState& OpenGLShader::appendDefaultPass() {
    _shaderPasses.push_back(new OpenGLShaderPass);
    OpenGLState& state = _shaderPasses.back()->state();
    return state;
}

// Test if we can render in bump map mode
bool OpenGLShader::canUseLightingMode() const
{
    return (
        GlobalShaderCache().lightingSupported()  // hw supports lighting mode
    	&& GlobalShaderCache().lightingEnabled()  // user enable lighting mode
    );
}

// Construct lighting mode render passes
void OpenGLShader::constructLightingPassesFromIShader()
{
    // Create depth-buffer fill pass
    OpenGLState& state = appendDefaultPass();
    state.renderFlags = RENDER_FILL 
                    | RENDER_CULLFACE 
                    | RENDER_TEXTURE 
                    | RENDER_DEPTHTEST 
                    | RENDER_DEPTHWRITE 
                    | RENDER_COLOURWRITE 
                    | RENDER_PROGRAM;

    state.m_colour[0] = 0;
    state.m_colour[1] = 0;
    state.m_colour[2] = 0;
    state.m_colour[3] = 1;
    state.m_sort = OpenGLState::eSortOpaque;
    
    state.m_program = render::GLProgramFactory::getProgram("depthFill").get();
    
    // Construct diffuse/bump/specular render pass
    OpenGLState& bumpPass = appendDefaultPass();
    bumpPass.m_texture = _iShader->getDiffuse()->texture_number;
    bumpPass.m_texture1 = _iShader->getBump()->texture_number;
    bumpPass.m_texture2 = _iShader->getSpecular()->texture_number;
    
    bumpPass.renderFlags = RENDER_BLEND
                       |RENDER_FILL
                       |RENDER_CULLFACE
                       |RENDER_DEPTHTEST
                       |RENDER_COLOURWRITE
                       |RENDER_SMOOTH
                       |RENDER_BUMP
                       |RENDER_PROGRAM;
    
    bumpPass.m_program = render::GLProgramFactory::getProgram("bumpMap").get();
    
    bumpPass.m_depthfunc = GL_LEQUAL;
    bumpPass.m_sort = OpenGLState::eSortMultiFirst;
    bumpPass.m_blend_src = GL_ONE;
    bumpPass.m_blend_dst = GL_ONE;

}

// Construct editor-image-only render passes
void OpenGLShader::constructEditorPreviewPassFromIShader()
{
    OpenGLState& state = appendDefaultPass();

    // Render the editor texture in legacy mode
    state.m_texture = _iShader->getEditorImage()->texture_number;
    state.renderFlags = RENDER_FILL
                    | RENDER_TEXTURE
                    |RENDER_DEPTHTEST
                    |RENDER_COLOURWRITE
                    |RENDER_LIGHTING
                    |RENDER_SMOOTH;

    // Handle certain shader flags
    if ((_iShader->getFlags() & QER_CULL) == 0
        || _iShader->getCull() == IShader::eCullBack)
    {
        state.renderFlags |= RENDER_CULLFACE;
    }

  if((_iShader->getFlags() & QER_ALPHATEST) != 0)
  {
    state.renderFlags |= RENDER_ALPHATEST;
    IShader::EAlphaFunc alphafunc;
    _iShader->getAlphaFunc(&alphafunc, &state.m_alpharef);
    switch(alphafunc)
    {
    case IShader::eAlways:
      state.m_alphafunc = GL_ALWAYS;
    case IShader::eEqual:
      state.m_alphafunc = GL_EQUAL;
    case IShader::eLess:
      state.m_alphafunc = GL_LESS;
    case IShader::eGreater:
      state.m_alphafunc = GL_GREATER;
    case IShader::eLEqual:
      state.m_alphafunc = GL_LEQUAL;
    case IShader::eGEqual:
      state.m_alphafunc = GL_GEQUAL;
    }
  }

    // Set the GL color
    reinterpret_cast<Vector3&>(state.m_colour) = _iShader->getEditorImage()->color;
    state.m_colour[3] = 1.0f;

    // Opaque blending, write to depth buffer
    state.renderFlags |= RENDER_DEPTHWRITE;
    state.m_sort = OpenGLState::eSortFullbright;
}

// Construct non-lighting mode render passes
void OpenGLShader::constructStandardPassesFromIShader()
{
    ShaderLayerVector allLayers(_iShader->getAllLayers());
    for (ShaderLayerVector::const_iterator i = allLayers.begin();
         i != allLayers.end();
         ++i)
    {
        TexturePtr layerTex = i->texture();

        OpenGLState& state = appendDefaultPass();
        state.renderFlags = RENDER_FILL
                        | RENDER_BLEND
                        | RENDER_TEXTURE
                        | RENDER_DEPTHTEST
                        | RENDER_COLOURWRITE;

        // Set the texture
        state.m_texture = layerTex->texture_number;

        // Get the blend function
        BlendFunc blendFunc = i->blendFunc();
        state.m_blend_src = blendFunc.src;
        state.m_blend_dst = blendFunc.dest;
        if(state.m_blend_src == GL_SRC_ALPHA || state.m_blend_dst == GL_SRC_ALPHA)
        {
          state.renderFlags |= RENDER_DEPTHWRITE;
        }

        // Colour modulation
        state.m_colour = Vector4(1.0, 0, 0.25, 1.0);

        state.m_sort = OpenGLState::eSortFullbright;
    }

#if 0 // legacy translucency code
    if((_iShader->getFlags() & QER_TRANS) != 0)
    {
        state.renderFlags |= RENDER_BLEND;
        state.m_colour[3] = _iShader->getTrans();
        state.m_sort = OpenGLState::eSortTranslucent;
        
        // Get the blend function
        BlendFunc blendFunc = _iShader->getBlendFunc();
        state.m_blend_src = blendFunc.m_src;
        state.m_blend_dst = blendFunc.m_dst;
        if(state.m_blend_src == GL_SRC_ALPHA || state.m_blend_dst == GL_SRC_ALPHA)
        {
          state.renderFlags |= RENDER_DEPTHWRITE;
        }
    }
    else
    {
        state.renderFlags |= RENDER_DEPTHWRITE;
        state.m_sort = OpenGLState::eSortFullbright;
    }
#endif
}

// Construct a normal shader
void OpenGLShader::constructNormalShader(const std::string& name)
{
    // Obtain the IShader
    _iShader = QERApp_Shader_ForName(name);

    // Determine whether we can render this shader in lighting/bump-map mode,
    // and construct the appropriate shader passes
    if (canUseLightingMode()) 
    {
        if (_iShader->getDiffuse())
        {
            // Regular light interaction
            constructLightingPassesFromIShader();
        }
        else
        {
            // Lighting mode without diffusemap, do multi-pass shading
            constructStandardPassesFromIShader();
        }
    }
    else
    {
        // Editor image rendering only
        constructEditorPreviewPassFromIShader();
    }
}

// Main shader construction entry point
void OpenGLShader::construct(const std::string& name)
{
	// Retrieve the highlight colour from the colourschemes (once)
	static Vector3 highLightColour = ColourSchemes().getColour("selected_brush_camera");
	
    // Check the first character of the name to see if this is a special built-in
    // shader
    switch(name[0])
    {
        case '(': // fill shader
        {
            OpenGLState& state = appendDefaultPass();
            sscanf(name.c_str(), "(%lf %lf %lf)", &state.m_colour[0], &state.m_colour[1], &state.m_colour[2]);
            state.m_colour[3] = 1.0f;
            state.renderFlags = RENDER_FILL|RENDER_LIGHTING|RENDER_DEPTHTEST|RENDER_CULLFACE|RENDER_COLOURWRITE|RENDER_DEPTHWRITE;
            state.m_sort = OpenGLState::eSortFullbright;
            break;
        }

        case '[':
        {
            OpenGLState& state = appendDefaultPass();
            sscanf(name.c_str(), "[%lf %lf %lf]", &state.m_colour[0], &state.m_colour[1], &state.m_colour[2]);
            state.m_colour[3] = 0.5f;
            state.renderFlags = RENDER_FILL|RENDER_LIGHTING|RENDER_DEPTHTEST|RENDER_CULLFACE|RENDER_COLOURWRITE|RENDER_DEPTHWRITE|RENDER_BLEND;
            state.m_sort = OpenGLState::eSortTranslucent;
            break;
        }

        case '<': // wireframe shader
        {
            OpenGLState& state = appendDefaultPass();
            sscanf(name.c_str(), "<%lf %lf %lf>", &state.m_colour[0], &state.m_colour[1], &state.m_colour[2]);
            state.m_colour[3] = 1;
            state.renderFlags = RENDER_DEPTHTEST|RENDER_COLOURWRITE|RENDER_DEPTHWRITE;
            state.m_sort = OpenGLState::eSortFullbright;
            state.m_depthfunc = GL_LESS;
            state.m_linewidth = 1;
            state.m_pointsize = 1;
            break;
        }

        case '$':
        {
            OpenGLState& state = appendDefaultPass();

            if (name == "$POINT")
            {
              state.renderFlags = RENDER_COLOURARRAY|RENDER_COLOURWRITE|RENDER_DEPTHWRITE;
              state.m_sort = OpenGLState::eSortControlFirst;
              state.m_pointsize = 4;
            }
            else if (name == "$SELPOINT")
            {
              state.renderFlags = RENDER_COLOURARRAY|RENDER_COLOURWRITE|RENDER_DEPTHWRITE;
              state.m_sort = OpenGLState::eSortControlFirst + 1;
              state.m_pointsize = 4;
            }
            else if (name == "$BIGPOINT")
            {
              state.renderFlags = RENDER_COLOURARRAY|RENDER_COLOURWRITE|RENDER_DEPTHWRITE;
              state.m_sort = OpenGLState::eSortControlFirst;
              state.m_pointsize = 6;
            }
            else if (name == "$PIVOT")
            {
              state.renderFlags = RENDER_COLOURARRAY|RENDER_COLOURWRITE|RENDER_DEPTHTEST|RENDER_DEPTHWRITE;
              state.m_sort = OpenGLState::eSortGUI1;
              state.m_linewidth = 2;
              state.m_depthfunc = GL_LEQUAL;

              OpenGLState& hiddenLine = appendDefaultPass();
              hiddenLine.renderFlags = RENDER_COLOURARRAY|RENDER_COLOURWRITE|RENDER_DEPTHTEST|RENDER_LINESTIPPLE;
              hiddenLine.m_sort = OpenGLState::eSortGUI0;
              hiddenLine.m_linewidth = 2;
              hiddenLine.m_depthfunc = GL_GREATER;
            }
            else if (name == "$LATTICE")
            {
              state.m_colour[0] = 1;
              state.m_colour[1] = 0.5;
              state.m_colour[2] = 0;
              state.m_colour[3] = 1;
              state.renderFlags = RENDER_COLOURWRITE|RENDER_DEPTHWRITE;
              state.m_sort = OpenGLState::eSortControlFirst;
            }
            else if (name == "$WIREFRAME")
            {
              state.renderFlags = RENDER_DEPTHTEST|RENDER_COLOURWRITE|RENDER_DEPTHWRITE;
              state.m_sort = OpenGLState::eSortFullbright;
            }
            else if (name == "$CAM_HIGHLIGHT")
            {
              state.m_colour[0] = highLightColour[0];
              state.m_colour[1] = highLightColour[1];
              state.m_colour[2] = highLightColour[2];
              state.m_colour[3] = 0.3f;
              state.renderFlags = RENDER_FILL|RENDER_DEPTHTEST|RENDER_CULLFACE|RENDER_BLEND|RENDER_COLOURWRITE|RENDER_DEPTHWRITE;
              state.m_sort = OpenGLState::eSortHighlight;
              state.m_depthfunc = GL_LEQUAL;
            }
            else if (name == "$CAM_OVERLAY")
            {
        #if 0
              state.renderFlags = RENDER_CULLFACE|RENDER_COLOURWRITE|RENDER_DEPTHWRITE;
              state.m_sort = OpenGLState::eSortOverlayFirst;
        #else
              state.renderFlags = RENDER_CULLFACE|RENDER_DEPTHTEST|RENDER_COLOURWRITE|RENDER_DEPTHWRITE|RENDER_OFFSETLINE;
              state.m_sort = OpenGLState::eSortOverlayFirst + 1;
              state.m_depthfunc = GL_LEQUAL;

              OpenGLState& hiddenLine = appendDefaultPass();
              hiddenLine.m_colour[0] = 0.75;
              hiddenLine.m_colour[1] = 0.75;
              hiddenLine.m_colour[2] = 0.75;
              hiddenLine.m_colour[3] = 1;
              hiddenLine.renderFlags = RENDER_CULLFACE|RENDER_DEPTHTEST|RENDER_COLOURWRITE|RENDER_OFFSETLINE|RENDER_LINESTIPPLE;
              hiddenLine.m_sort = OpenGLState::eSortOverlayFirst;
              hiddenLine.m_depthfunc = GL_GREATER;
              hiddenLine.m_linestipple_factor = 2;
        #endif
            }
            else if (name == "$XY_OVERLAY")
            {
              Vector3 colorSelBrushes = ColourSchemes().getColour("selected_brush");
              state.m_colour[0] = colorSelBrushes[0];
              state.m_colour[1] = colorSelBrushes[1];
              state.m_colour[2] = colorSelBrushes[2];
              state.m_colour[3] = 1;
              state.renderFlags = RENDER_COLOURWRITE | RENDER_LINESTIPPLE;
              state.m_sort = OpenGLState::eSortOverlayFirst;
              state.m_linewidth = 2;
              state.m_linestipple_factor = 3;
            }
            else if (name == "$DEBUG_CLIPPED")
            {
              state.renderFlags = RENDER_COLOURARRAY | RENDER_COLOURWRITE | RENDER_DEPTHWRITE;
              state.m_sort = OpenGLState::eSortLast;
            }
            else if (name == "$POINTFILE")
            {
              state.m_colour[0] = 1;
              state.m_colour[1] = 0;
              state.m_colour[2] = 0;
              state.m_colour[3] = 1;
              state.renderFlags = RENDER_DEPTHTEST | RENDER_COLOURWRITE | RENDER_DEPTHWRITE;
              state.m_sort = OpenGLState::eSortFullbright;
              state.m_linewidth = 4;
            }
            else if (name == "$LIGHT_SPHERE")
            {
              state.m_colour[0] = .15f * .95f;
              state.m_colour[1] = .15f * .95f;
              state.m_colour[2] = .15f * .95f;
              state.m_colour[3] = 1;
              state.renderFlags = RENDER_CULLFACE | RENDER_DEPTHTEST | RENDER_BLEND | RENDER_FILL | RENDER_COLOURWRITE | RENDER_DEPTHWRITE;
              state.m_blend_src = GL_ONE;
              state.m_blend_dst = GL_ONE;
              state.m_sort = OpenGLState::eSortTranslucent;
            }
            else if (name == "$Q3MAP2_LIGHT_SPHERE")
            {
              state.m_colour[0] = .05f;
              state.m_colour[1] = .05f;
              state.m_colour[2] = .05f;
              state.m_colour[3] = 1;
              state.renderFlags = RENDER_CULLFACE | RENDER_DEPTHTEST | RENDER_BLEND | RENDER_FILL;
              state.m_blend_src = GL_ONE;
              state.m_blend_dst = GL_ONE;
              state.m_sort = OpenGLState::eSortTranslucent;
            }
            else if (name == "$WIRE_OVERLAY")
            {
        #if 0
              state.renderFlags = RENDER_COLOURARRAY | RENDER_COLOURWRITE | RENDER_DEPTHWRITE | RENDER_DEPTHTEST | RENDER_OVERRIDE;
              state.m_sort = OpenGLState::eSortOverlayFirst;
        #else
              state.renderFlags = RENDER_COLOURARRAY | RENDER_COLOURWRITE | RENDER_DEPTHWRITE | RENDER_DEPTHTEST | RENDER_OVERRIDE;
              state.m_sort = OpenGLState::eSortGUI1;
              state.m_depthfunc = GL_LEQUAL;

              OpenGLState& hiddenLine = appendDefaultPass();
              hiddenLine.renderFlags = RENDER_COLOURARRAY | RENDER_COLOURWRITE | RENDER_DEPTHWRITE | RENDER_DEPTHTEST | RENDER_OVERRIDE | RENDER_LINESTIPPLE;
              hiddenLine.m_sort = OpenGLState::eSortGUI0;
              hiddenLine.m_depthfunc = GL_GREATER;
        #endif
            }
            else if (name == "$FLATSHADE_OVERLAY")
            {
              state.renderFlags = RENDER_CULLFACE | RENDER_LIGHTING | RENDER_SMOOTH | RENDER_SCALED | RENDER_COLOURARRAY | RENDER_FILL | RENDER_COLOURWRITE | RENDER_DEPTHWRITE | RENDER_DEPTHTEST | RENDER_OVERRIDE;
              state.m_sort = OpenGLState::eSortGUI1;
              state.m_depthfunc = GL_LEQUAL;

              OpenGLState& hiddenLine = appendDefaultPass();
              hiddenLine.renderFlags = RENDER_CULLFACE | RENDER_LIGHTING | RENDER_SMOOTH | RENDER_SCALED | RENDER_COLOURARRAY | RENDER_FILL | RENDER_COLOURWRITE | RENDER_DEPTHWRITE | RENDER_DEPTHTEST | RENDER_OVERRIDE | RENDER_POLYGONSTIPPLE;
              hiddenLine.m_sort = OpenGLState::eSortGUI0;
              hiddenLine.m_depthfunc = GL_GREATER;
            }
            else if (name == "$CLIPPER_OVERLAY")
            {
              Vector3 colorClipper = ColourSchemes().getColour("clipper");
              state.m_colour[0] = colorClipper[0];
              state.m_colour[1] = colorClipper[1];
              state.m_colour[2] = colorClipper[2];
              state.m_colour[3] = 1;
              state.renderFlags = RENDER_CULLFACE | RENDER_COLOURWRITE | RENDER_DEPTHWRITE | RENDER_FILL | RENDER_POLYGONSTIPPLE;
              state.m_sort = OpenGLState::eSortOverlayFirst;
            }
            else if (name == "$OVERBRIGHT")
            {
              const float lightScale = 2;
              state.m_colour[0] = lightScale * 0.5f;
              state.m_colour[1] = lightScale * 0.5f;
              state.m_colour[2] = lightScale * 0.5f;
              state.m_colour[3] = 0.5;
              state.renderFlags = RENDER_FILL|RENDER_BLEND|RENDER_COLOURWRITE|RENDER_SCREEN;
              state.m_sort = OpenGLState::eSortOverbrighten;
              state.m_blend_src = GL_DST_COLOR;
              state.m_blend_dst = GL_SRC_COLOR;
            }
            else
            {
              // default to something recognisable.. =)
              ERROR_MESSAGE("hardcoded renderstate not found");
              state.m_colour[0] = 1;
              state.m_colour[1] = 0;
              state.m_colour[2] = 1;
              state.m_colour[3] = 1;
              state.renderFlags = RENDER_COLOURWRITE | RENDER_DEPTHWRITE;
              state.m_sort = OpenGLState::eSortFirst;
            }
            break;
        } // case '$'

        default:
        {
            // This is not a hard-coded shader, construct from the shader system
            constructNormalShader(name);
        }

    } // switch
}

