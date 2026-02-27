//
// Created by Monika on 26.01.2024.
//

#include <Graphics/SRSL/ShaderVariables.h>
#include <Graphics/Pipeline/IShaderProgram.h>

#include <Enum/ShaderStage.hpp>

namespace SR_SRSL_NS {
    std::string ShaderRenderPassTypeToString(ShaderRenderPassType type) {
        return "RenderPassType_" + SR_UTILS_NS::EnumReflector::ToStringAtom(type).ToString();
    }

    const std::map<std::string, std::string> SR_SRSL_DEFAULT_PUSH_CONSTANTS = { /** NOLINT */
        { "PC_SHADOW_CASCADE_INDEX",           "int"        },
        { "PC_COLOR_BUFFER_MODE",              "int"        },
        { "COMPUTE_STAGE",                     "int"        },
        { "PC_COLOR_BUFFER_VALUE",             "vec3"       },
    };

    const std::map<std::string, std::string> SR_SRSL_DEFAULT_SHARED_UNIFORMS = { /** NOLINT */
        { "VIEW_MATRIX",                    "mat4"          },
        { "INVERSE_VIEW_MATRIX",            "mat4"          },
        { "PROJECTION_MATRIX",              "mat4"          },
        { "INVERSE_PROJECTION_MATRIX",      "mat4"          },
        { "PROJECTION_NO_FOV_MATRIX",       "mat4"          },
        { "ORTHOGONAL_MATRIX",              "mat4"          },
        { "PIXEL_ORTHOGONAL_MATRIX",        "mat4"          },
        { "VIEW_NO_TRANSLATE_MATRIX",       "mat4"          },
        { "LIGHT_SPACE_MATRIX",             "mat4"          },

        { "TIME",                           "float"         },
        { "SUN_INTENSITY",                  "float"         },
        { "SHADOW_STRENGTH",                "float"         },
        { "AMBIENT_INTENSITY",              "float"         },
        { "CAMERA_FAR",                     "float"         },
        { "CAMERA_NEAR",                    "float"         },

        { "RENDER_PASS_TYPE",               "int"           },

        { "RESOLUTION",                     "vec2"          },
        { "ASPECT",                         "vec2"          },

        { "CASCADE_LIGHT_SPACE_MATRICES",   "mat4[4]"       },
        { "CASCADE_RADII",                  "vec4"          },
        { "CASCADE_SPLITS",                 "vec4"          },

        { "CASCADE_CENTERS",                "vec3[4]"       },
        { "DIRECTIONAL_LIGHT_DIRECTION",    "vec3"          },
        { "VIEW_POSITION",                  "vec3"          },
        { "VIEW_DIRECTION",                 "vec3"          },
        { "SUN_COLOR",                      "vec3"          },
        { "SKY_COLOR",                      "vec3"          },
        { "GROUND_COLOR",                   "vec3"          },
    };

    const std::map<std::string, std::string>& GetDefaultUniforms() {
        static const std::map<std::string, std::string> defaultUniforms = { /** NOLINT */
            { "MODEL_MATRIX",                   "mat4"          },
            { "MODEL_NO_SCALE_MATRIX",          "mat4"          },

            { "SKELETON_MATRICES_128",          "mat4[128]"     },
            { "SKELETON_MATRIX_OFFSETS_128",    "mat4[128]"     },

            { "SKELETON_MATRICES_256",          "mat4[256]"     },
            { "SKELETON_MATRIX_OFFSETS_256",    "mat4[256]"     },

            { "SKELETON_MATRICES_384",          "mat4[384]"     },
            { "SKELETON_MATRIX_OFFSETS_384",    "mat4[384]"     },

            { "HALF_SIZE_NEAR_PLANE",           "vec2"          },
            { "TEXT_ATLAS_SIZE",                "vec2"          },

            { "LINE_START_POINT",               "vec3"          },
            { "LINE_END_POINT",                 "vec3"          },

            { "SSAO_SAMPLES",                   "vec4[64]"      },

            { "LINE_COLOR",                     "vec4"          },
            { "RGBA_VALUE",                     "vec4"          },
            { "SLICED_TEXTURE_BORDER",          "vec4"          },
            { "SLICED_WINDOW_BORDER",           "vec4"          },

            { "TEXT_RECT_X",                    "float"         },
            { "TEXT_RECT_Y",                    "float"         },
            { "TEXT_RECT_WIDTH",                "float"         },
            { "TEXT_RECT_HEIGHT",               "float"         },
            { "SPRITE_FILL_AMOUNT",             "float"         },

            { "FILL_CENTER",                    "int"           },
            { "SPRITE_MODE",                    "int"           },
            { "SPRITE_FILL_ORIGIN",             "int"           },
            { "SPRITE_FILL_METHOD",             "int"           },
            { "SPRITE_FILL_CLOCKWISE",          "int"           },
        };

        return defaultUniforms;
    }

    const std::map<std::string, std::string> SR_SRSL_DEFAULT_SAMPLERS = { /** NOLINT */
        { "SKYBOX_DIFFUSE",                 "samplerCube"   },
        { "TEXT_ATLAS_TEXTURE",             "sampler2D"     },
        { "SSAO_NOISE",                     "sampler2D"     },
    };

    const std::string SR_SRSL_MAIN_OUT_LAYER = "COLOR_INDEX_0"; /** NOLINT */

    const std::set<std::string> SR_SRSL_DEFAULT_OUT_LAYERS = { /** NOLINT */
        { "COLOR_INDEX_0" },
        { "COLOR_INDEX_1" },
        { "COLOR_INDEX_2" },
        { "COLOR_INDEX_3" },
        { "COLOR_INDEX_4" },
        { "COLOR_INDEX_5" },
        { "COLOR_INDEX_6" },
        { "COLOR_INDEX_7" },
        { "COLOR_INDEX_8" },
    };

    const std::vector<std::string> SR_SRSL_DEFAULT_OUT_LAYERS_USE_MACRO = { /** NOLINT */
        { "USE_COLOR_INDEX_0" },
        { "USE_COLOR_INDEX_1" },
        { "USE_COLOR_INDEX_2" },
        { "USE_COLOR_INDEX_3" },
        { "USE_COLOR_INDEX_4" },
        { "USE_COLOR_INDEX_5" },
        { "USE_COLOR_INDEX_6" },
        { "USE_COLOR_INDEX_7" },
        { "USE_COLOR_INDEX_8" },
    };

    const std::map<ShaderStage, std::string> SR_SRSL_ENTRY_POINTS = { /** NOLINT */
        { ShaderStage::Vertex,   "vertex"    },
        { ShaderStage::Fragment, "fragment"  },
        { ShaderStage::Compute,  "compute"   },
    };

    const std::map<ShaderStage, std::string> SR_SRSL_STAGE_EXTENSIONS = { /** NOLINT */
        { ShaderStage::Vertex,        "vert"        },
        { ShaderStage::Fragment,      "frag"        },
        { ShaderStage::Compute,       "comp"        },
        { ShaderStage::Raygen,        "rgen"        },
        { ShaderStage::Intersection,  "rint"        },
        { ShaderStage::HitClosest,    "rchit"       },
        { ShaderStage::HitAny,        "rahit"       },
        { ShaderStage::MissPrimary  , "rmiss"       },
        { ShaderStage::MissSecondary, "rmiss"       },
    };

    const std::map<std::string, ShaderVarType> SR_SRSL_TYPE_STRINGS = { /** NOLINT */
        { "bool",               ShaderVarType::Bool             },

        { "int",                ShaderVarType::Int              },
        { "float",              ShaderVarType::Float            },

        { "bvec2",              ShaderVarType::BVec2            },
        { "bvec3",              ShaderVarType::BVec3            },
        { "bvec4",              ShaderVarType::BVec4            },

        { "ivec2",              ShaderVarType::IVec2            },
        { "ivec3",              ShaderVarType::IVec3            },
        { "ivec4",              ShaderVarType::IVec4            },

        { "vec2",               ShaderVarType::Vec2             },
        { "vec3",               ShaderVarType::Vec3             },
        { "vec4",               ShaderVarType::Vec4             },

        { "mat2",               ShaderVarType::Mat2             },
        { "mat3",               ShaderVarType::Mat3             },
        { "mat4",               ShaderVarType::Mat4             },

        { "sampler1D",          ShaderVarType::Sampler1D        },
        { "sampler2D",          ShaderVarType::Sampler2D        },
        { "sampler2DMS",        ShaderVarType::Sampler2D        },
        { "sampler3D",          ShaderVarType::Sampler3D        },
        { "sampler1DShadow",    ShaderVarType::Sampler1DShadow  },
        { "sampler2DShadow",    ShaderVarType::Sampler2DShadow  },
        { "samplerCube",        ShaderVarType::SamplerCube      },
    };

    const std::map<std::string, uint64_t> SR_SRSL_TYPE_SIZE_TABLE = { /** NOLINT */
        { "bool",         4         },

        { "uint",         4         },
        { "int",          4         },
        { "float",        4         },

        { "bvec2",        1 * 2     },
        { "bvec3",        1 * 3     },
        { "bvec4",        1 * 4     },

        { "ivec2",        4 * 2     },
        { "ivec3",        4 * 3     },
        { "ivec4",        4 * 4     },

        { "vec2",         4 * 2     },
        { "vec3",         4 * 3     },
        { "vec4",         4 * 4     },

        { "mat2",         4 * 2 * 2 },
        { "mat3",         4 * 3 * 3 },
        { "mat4",         4 * 4 * 4 },
    };
}
