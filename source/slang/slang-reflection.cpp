// slang-reflection.cpp
#include "slang-reflection.h"

#include "slang-compiler.h"
#include "slang-type-layout.h"
#include "slang-syntax.h"
#include <assert.h>

// Don't signal errors for stuff we don't implement here,
// and instead just try to return things defensively
//
// Slang developers can switch this when debugging.
#define SLANG_REFLECTION_UNEXPECTED() do {} while(0)

// Implementation to back public-facing reflection API

namespace Slang {
namespace Reflection {

// Conversion routines to help with strongly-typed reflection API
static inline Session* convert(SlangSession* session)
{
    return (Session*)session;
}

static inline UserDefinedAttribute* convert(SlangReflectionUserAttribute* attrib)
{
    return (UserDefinedAttribute*)attrib;
}
static inline SlangReflectionUserAttribute* convert(UserDefinedAttribute* attrib)
{
    return (SlangReflectionUserAttribute*)attrib;
}
static inline Type* convert(SlangReflectionType* type)
{
    return (Type*) type;
}

static inline SlangReflectionType* convert(Type* type)
{
    return (SlangReflectionType*) type;
}

static inline TypeLayout* convert(SlangReflectionTypeLayout* type)
{
    return (TypeLayout*) type;
}

static inline SlangReflectionTypeLayout* convert(TypeLayout* type)
{
    return (SlangReflectionTypeLayout*) type;
}

static inline SpecializationParamLayout* convert(SlangReflectionTypeParameter * typeParam)
{
    return (SpecializationParamLayout*) typeParam;
}

static inline VarDeclBase* convert(SlangReflectionVariable* var)
{
    return (VarDeclBase*) var;
}

static inline SlangReflectionVariable* convert(VarDeclBase* var)
{
    return (SlangReflectionVariable*) var;
}

static inline VarLayout* convert(SlangReflectionVariableLayout* var)
{
    return (VarLayout*) var;
}

static inline SlangReflectionVariableLayout* convert(VarLayout* var)
{
    return (SlangReflectionVariableLayout*) var;
}

static inline EntryPointLayout* convert(SlangReflectionEntryPoint* entryPoint)
{
    return (EntryPointLayout*) entryPoint;
}

static inline SlangReflectionEntryPoint* convert(EntryPointLayout* entryPoint)
{
    return (SlangReflectionEntryPoint*) entryPoint;
}

static inline ProgramLayout* convert(SlangReflection* program)
{
    return (ProgramLayout*) program;
}

static inline SlangReflection* convert(ProgramLayout* program)
{
    return (SlangReflection*) program;
}

// user attaribute

static unsigned int getUserAttributeCount(Decl* decl)
{
    unsigned int count = 0;
    for (auto x : decl->getModifiersOfType<UserDefinedAttribute>())
    {
        SLANG_UNUSED(x);
        count++;
    }
    return count;
}

static SlangReflectionUserAttribute* findUserAttributeByName(Session* session, Decl* decl, const char* name)
{
    auto nameObj = session->tryGetNameObj(name);
    for (auto x : decl->getModifiersOfType<UserDefinedAttribute>())
    {
        if (x->name == nameObj)
            return (SlangReflectionUserAttribute*)(x);
    }
    return nullptr;
}

static SlangReflectionUserAttribute* getUserAttributeByIndex(Decl* decl, unsigned int index)
{
    unsigned int id = 0;
    for (auto x : decl->getModifiersOfType<UserDefinedAttribute>())
    {
        if (id == index)
            return convert(x);
        id++;
    }
    return nullptr;
}

static size_t getReflectionSize(LayoutSize size)
{
    if(size.isFinite())
        return size.getFiniteValue();

    return SLANG_UNBOUNDED_SIZE;
}

} // namespace Reflection

static SlangParameterCategory getParameterCategory(
    LayoutResourceKind kind)
{
    return SlangParameterCategory(kind);
}

static SlangParameterCategory getParameterCategory(
    TypeLayout*  typeLayout)
{
    auto resourceInfoCount = typeLayout->resourceInfos.getCount();
    if(resourceInfoCount == 1)
    {
        return getParameterCategory(typeLayout->resourceInfos[0].kind);
    }
    else if(resourceInfoCount == 0)
    {
        // TODO: can this ever happen?
        return SLANG_PARAMETER_CATEGORY_NONE;
    }
    return SLANG_PARAMETER_CATEGORY_MIXED;
}

static TypeLayout* maybeGetContainerLayout(TypeLayout* typeLayout)
{
    if (auto parameterGroupTypeLayout = as<ParameterGroupTypeLayout>(typeLayout))
    {
        auto containerTypeLayout = parameterGroupTypeLayout->containerVarLayout->typeLayout;
        if (containerTypeLayout->resourceInfos.getCount() != 0)
        {
            return containerTypeLayout;
        }
    }

    return typeLayout;
}

static bool hasDefaultConstantBuffer(ScopeLayout* layout)
{
    auto typeLayout = layout->parametersLayout->getTypeLayout();
    return as<ParameterGroupTypeLayout>(typeLayout) != nullptr;
}

} // namespace Slang

char const* spReflectionUserAttribute_GetName(SlangReflectionUserAttribute* attrib)
{
    using namespace Slang;
    using namespace Reflection;

    auto userAttr = convert(attrib);
    if (!userAttr) return nullptr;
    return userAttr->getName()->text.getBuffer();
}
unsigned int spReflectionUserAttribute_GetArgumentCount(SlangReflectionUserAttribute* attrib)
{
    using namespace Slang;
    using namespace Reflection;

    auto userAttr = convert(attrib);
    if (!userAttr) return 0;
    return (unsigned int)userAttr->args.getCount();
}
SlangReflectionType* spReflectionUserAttribute_GetArgumentType(SlangReflectionUserAttribute* attrib, unsigned int index)
{
    using namespace Slang;
    using namespace Reflection;

    auto userAttr = convert(attrib);
    if (!userAttr) return nullptr;
    return convert(userAttr->args[index]->type.type);
}
SlangResult spReflectionUserAttribute_GetArgumentValueInt(SlangReflectionUserAttribute* attrib, unsigned int index, int * rs)
{
    using namespace Slang;
    using namespace Reflection;

    auto userAttr = convert(attrib);
    if (!userAttr) return SLANG_ERROR_INVALID_PARAMETER;
    if (index >= (unsigned int)userAttr->args.getCount()) return SLANG_ERROR_INVALID_PARAMETER;
    NodeBase* val = nullptr;
    if (userAttr->intArgVals.TryGetValue(index, val))
    {
        *rs = (int)as<ConstantIntVal>(val)->value;
        return 0;
    }
    return SLANG_ERROR_INVALID_PARAMETER;
}
SlangResult spReflectionUserAttribute_GetArgumentValueFloat(SlangReflectionUserAttribute* attrib, unsigned int index, float * rs)
{
    using namespace Slang;
    using namespace Reflection;

    auto userAttr = convert(attrib);
    if (!userAttr) return SLANG_ERROR_INVALID_PARAMETER;
    if (index >= (unsigned int)userAttr->args.getCount()) return SLANG_ERROR_INVALID_PARAMETER;
    if (auto cexpr = as<FloatingPointLiteralExpr>(userAttr->args[index]))
    {
        *rs = (float)cexpr->value;
        return 0;
    }
    return SLANG_ERROR_INVALID_PARAMETER;
}
const char* spReflectionUserAttribute_GetArgumentValueString(SlangReflectionUserAttribute* attrib, unsigned int index, size_t* bufLen)
{
    using namespace Slang;
    using namespace Reflection;

    auto userAttr = convert(attrib);
    if (!userAttr) return nullptr;
    if (index >= (unsigned int)userAttr->args.getCount()) return nullptr;
    if (auto cexpr = as<StringLiteralExpr>(userAttr->args[index]))
    {
        if (bufLen)
            *bufLen = cexpr->token.getContentLength();
        return cexpr->token.getContent().begin();
    }
    return nullptr;
}



// type Reflection


SlangTypeKind spReflectionType_GetKind(SlangReflectionType* inType)
{
    using namespace Slang;
    using namespace Reflection;

    auto type = convert(inType);
    if(!type) return SLANG_TYPE_KIND_NONE;

    // TODO(tfoley: Don't emit the same type more than once...

    if (auto basicType = as<BasicExpressionType>(type))
    {
        return SLANG_TYPE_KIND_SCALAR;
    }
    else if (auto vectorType = as<VectorExpressionType>(type))
    {
        return SLANG_TYPE_KIND_VECTOR;
    }
    else if (auto matrixType = as<MatrixExpressionType>(type))
    {
        return SLANG_TYPE_KIND_MATRIX;
    }
    else if (auto parameterBlockType = as<ParameterBlockType>(type))
    {
        return SLANG_TYPE_KIND_PARAMETER_BLOCK;
    }
    else if (auto constantBufferType = as<ConstantBufferType>(type))
    {
        return SLANG_TYPE_KIND_CONSTANT_BUFFER;
    }
    else if( auto streamOutputType = as<HLSLStreamOutputType>(type) )
    {
        return SLANG_TYPE_KIND_OUTPUT_STREAM;
    }
    else if (as<TextureBufferType>(type))
    {
        return SLANG_TYPE_KIND_TEXTURE_BUFFER;
    }
    else if (as<GLSLShaderStorageBufferType>(type))
    {
        return SLANG_TYPE_KIND_SHADER_STORAGE_BUFFER;
    }
    else if (auto samplerStateType = as<SamplerStateType>(type))
    {
        return SLANG_TYPE_KIND_SAMPLER_STATE;
    }
    else if (auto textureType = as<TextureTypeBase>(type))
    {
        return SLANG_TYPE_KIND_RESOURCE;
    }
    else if (auto feedbackType = as<FeedbackType>(type))
    {
        return SLANG_TYPE_KIND_FEEDBACK;
    }
    // TODO: need a better way to handle this stuff...
#define CASE(TYPE)                          \
    else if(as<TYPE>(type)) do {          \
        return SLANG_TYPE_KIND_RESOURCE;    \
    } while(0)

    CASE(HLSLStructuredBufferType);
    CASE(HLSLRWStructuredBufferType);
    CASE(HLSLRasterizerOrderedStructuredBufferType);
    CASE(HLSLAppendStructuredBufferType);
    CASE(HLSLConsumeStructuredBufferType);
    CASE(HLSLByteAddressBufferType);
    CASE(HLSLRWByteAddressBufferType);
    CASE(HLSLRasterizerOrderedByteAddressBufferType);
    CASE(UntypedBufferResourceType);
#undef CASE

    else if (auto arrayType = as<ArrayExpressionType>(type))
    {
        return SLANG_TYPE_KIND_ARRAY;
    }
    else if( auto declRefType = as<DeclRefType>(type) )
    {
        const auto& declRef = declRefType->declRef;
        if(declRef.is<StructDecl>() )
        {
            return SLANG_TYPE_KIND_STRUCT;
        }
        else if (declRef.is<GlobalGenericParamDecl>())
        {
            return SLANG_TYPE_KIND_GENERIC_TYPE_PARAMETER;
        }
        else if (declRef.is<InterfaceDecl>())
        {
            return SLANG_TYPE_KIND_INTERFACE;
        }
    }
    else if( auto specializedType = as<ExistentialSpecializedType>(type) )
    {
        return SLANG_TYPE_KIND_SPECIALIZED;
    }
    else if (auto errorType = as<ErrorType>(type))
    {
        // This means we saw a type we didn't understand in the user's code
        return SLANG_TYPE_KIND_NONE;
    }

    SLANG_REFLECTION_UNEXPECTED();
    return SLANG_TYPE_KIND_NONE;
}

unsigned int spReflectionType_GetFieldCount(SlangReflectionType* inType)
{
    using namespace Slang;
    using namespace Reflection;

    auto type = convert(inType);
    if(!type) return 0;

    // TODO: maybe filter based on kind

    if(auto declRefType = as<DeclRefType>(type))
    {
        auto declRef = declRefType->declRef;
        if( auto structDeclRef = declRef.as<StructDecl>())
        {
            return (unsigned int)getFields(structDeclRef, MemberFilterStyle::Instance).getCount();
        }
    }

    return 0;
}

SlangReflectionVariable* spReflectionType_GetFieldByIndex(SlangReflectionType* inType, unsigned index)
{
    using namespace Slang;
    using namespace Reflection;

    auto type = convert(inType);
    if(!type) return nullptr;

    // TODO: maybe filter based on kind

    if(auto declRefType = as<DeclRefType>(type))
    {
        auto declRef = declRefType->declRef;
        if( auto structDeclRef = declRef.as<StructDecl>())
        {
            auto fields = getFields(structDeclRef, MemberFilterStyle::Instance);
            auto fieldDeclRef = fields[index];
            return (SlangReflectionVariable*) fieldDeclRef.getDecl();
        }
    }

    return nullptr;
}

size_t spReflectionType_GetElementCount(SlangReflectionType* inType)
{
    using namespace Slang;
    using namespace Reflection;

    auto type = convert(inType);
    if(!type) return 0;

    if(auto arrayType = as<ArrayExpressionType>(type))
    {
        return arrayType->arrayLength ? (size_t) getIntVal(arrayType->arrayLength) : 0;
    }
    else if( auto vectorType = as<VectorExpressionType>(type))
    {
        return (size_t) getIntVal(vectorType->elementCount);
    }

    return 0;
}

SlangReflectionType* spReflectionType_GetElementType(SlangReflectionType* inType)
{
    using namespace Slang;
    using namespace Reflection;

    auto type = convert(inType);
    if(!type) return nullptr;

    if(auto arrayType = as<ArrayExpressionType>(type))
    {
        return (SlangReflectionType*) arrayType->baseType;
    }
    else if( auto parameterGroupType = as<ParameterGroupType>(type))
    {
        return convert(parameterGroupType->elementType);
    }
    else if( auto vectorType = as<VectorExpressionType>(type))
    {
        return convert(vectorType->elementType);
    }
    else if( auto matrixType = as<MatrixExpressionType>(type))
    {
        return convert(matrixType->getElementType());
    }

    return nullptr;
}

unsigned int spReflectionType_GetRowCount(SlangReflectionType* inType)
{
    using namespace Slang;
    using namespace Reflection;

    auto type = convert(inType);
    if(!type) return 0;

    if(auto matrixType = as<MatrixExpressionType>(type))
    {
        return (unsigned int) getIntVal(matrixType->getRowCount());
    }
    else if(auto vectorType = as<VectorExpressionType>(type))
    {
        return 1;
    }
    else if( auto basicType = as<BasicExpressionType>(type) )
    {
        return 1;
    }

    return 0;
}

unsigned int spReflectionType_GetColumnCount(SlangReflectionType* inType)
{
    using namespace Slang;
    using namespace Reflection;

    auto type = convert(inType);
    if(!type) return 0;

    if(auto matrixType = as<MatrixExpressionType>(type))
    {
        return (unsigned int) getIntVal(matrixType->getColumnCount());
    }
    else if(auto vectorType = as<VectorExpressionType>(type))
    {
        return (unsigned int) getIntVal(vectorType->elementCount);
    }
    else if( auto basicType = as<BasicExpressionType>(type) )
    {
        return 1;
    }

    return 0;
}

SlangScalarType spReflectionType_GetScalarType(SlangReflectionType* inType)
{
    using namespace Slang;
    using namespace Reflection;

    auto type = convert(inType);
    if(!type) return 0;

    if(auto matrixType = as<MatrixExpressionType>(type))
    {
        type = matrixType->getElementType();
    }
    else if(auto vectorType = as<VectorExpressionType>(type))
    {
        type = vectorType->elementType;
    }

    if(auto basicType = as<BasicExpressionType>(type))
    {
        switch (basicType->baseType)
        {
#define CASE(BASE, TAG) \
        case BaseType::BASE: return SLANG_SCALAR_TYPE_##TAG

            CASE(Void,      VOID);
            CASE(Bool,      BOOL);
            CASE(Int8,      INT8);
            CASE(Int16,     INT16);
            CASE(Int,       INT32);
            CASE(Int64,     INT64);
            CASE(UInt8,     UINT8);
            CASE(UInt16,    UINT16);
            CASE(UInt,      UINT32);
            CASE(UInt64,    UINT64);
            CASE(Half,      FLOAT16);
            CASE(Float,     FLOAT32);
            CASE(Double,    FLOAT64);

#undef CASE

        default:
            SLANG_REFLECTION_UNEXPECTED();
            return SLANG_SCALAR_TYPE_NONE;
            break;
        }
    }

    return SLANG_SCALAR_TYPE_NONE;
}

unsigned int spReflectionType_GetUserAttributeCount(SlangReflectionType* inType)
{
    using namespace Slang;
    using namespace Reflection;

    auto type = convert(inType);
    if (!type) return 0;
    if (auto declRefType = as<DeclRefType>(type))
    {
        return getUserAttributeCount(declRefType->declRef.getDecl());
    }
    return 0;
}
SlangReflectionUserAttribute* spReflectionType_GetUserAttribute(SlangReflectionType* inType, unsigned int index)
{
    using namespace Slang;
    using namespace Reflection;

    auto type = convert(inType);
    if (!type) return 0;
    if (auto declRefType = as<DeclRefType>(type))
    {
        return getUserAttributeByIndex(declRefType->declRef.getDecl(), index);
    }
    return 0;
}
SlangReflectionUserAttribute* spReflectionType_FindUserAttributeByName(SlangReflectionType* inType, char const* name)
{
    using namespace Slang;
    using namespace Reflection;

    auto type = convert(inType);
    if (!type) return 0;
    if (auto declRefType = as<DeclRefType>(type))
    {
        ASTBuilder* astBuilder = declRefType->getASTBuilder();
        auto globalSession = astBuilder->getGlobalSession();

        return findUserAttributeByName(globalSession, declRefType->declRef.getDecl(), name);
    }
    return 0;
}

SlangResourceShape spReflectionType_GetResourceShape(SlangReflectionType* inType)
{
    using namespace Slang;
    using namespace Reflection;

    auto type = convert(inType);
    if(!type) return 0;

    while(auto arrayType = as<ArrayExpressionType>(type))
    {
        type = arrayType->baseType;
    }

    if(auto textureType = as<TextureTypeBase>(type))
    {
        return textureType->getShape();
    }

    // TODO: need a better way to handle this stuff...
#define CASE(TYPE, SHAPE, ACCESS)   \
    else if(as<TYPE>(type)) do {  \
        return SHAPE;               \
    } while(0)

    CASE(HLSLStructuredBufferType,                      SLANG_STRUCTURED_BUFFER,        SLANG_RESOURCE_ACCESS_READ);
    CASE(HLSLRWStructuredBufferType,                    SLANG_STRUCTURED_BUFFER,        SLANG_RESOURCE_ACCESS_READ_WRITE);
    CASE(HLSLRasterizerOrderedStructuredBufferType,     SLANG_STRUCTURED_BUFFER,        SLANG_RESOURCE_ACCESS_RASTER_ORDERED);
    CASE(HLSLAppendStructuredBufferType,                SLANG_STRUCTURED_BUFFER,        SLANG_RESOURCE_ACCESS_APPEND);
    CASE(HLSLConsumeStructuredBufferType,               SLANG_STRUCTURED_BUFFER,        SLANG_RESOURCE_ACCESS_CONSUME);
    CASE(HLSLByteAddressBufferType,                     SLANG_BYTE_ADDRESS_BUFFER,      SLANG_RESOURCE_ACCESS_READ);
    CASE(HLSLRWByteAddressBufferType,                   SLANG_BYTE_ADDRESS_BUFFER,      SLANG_RESOURCE_ACCESS_READ_WRITE);
    CASE(HLSLRasterizerOrderedByteAddressBufferType,    SLANG_BYTE_ADDRESS_BUFFER,      SLANG_RESOURCE_ACCESS_RASTER_ORDERED);
    CASE(RaytracingAccelerationStructureType,           SLANG_ACCELERATION_STRUCTURE,   SLANG_RESOURCE_ACCESS_READ);
    CASE(UntypedBufferResourceType,                     SLANG_BYTE_ADDRESS_BUFFER,      SLANG_RESOURCE_ACCESS_READ);
#undef CASE

    return SLANG_RESOURCE_NONE;
}

SlangResourceAccess spReflectionType_GetResourceAccess(SlangReflectionType* inType)
{
    using namespace Slang;
    using namespace Reflection;

    auto type = convert(inType);
    if(!type) return 0;

    while(auto arrayType = as<ArrayExpressionType>(type))
    {
        type = arrayType->baseType;
    }

    if(auto textureType = as<TextureTypeBase>(type))
    {
        return textureType->getAccess();
    }

    // TODO: need a better way to handle this stuff...
#define CASE(TYPE, SHAPE, ACCESS)   \
    else if(as<TYPE>(type)) do {  \
        return ACCESS;              \
    } while(0)

    CASE(HLSLStructuredBufferType,                      SLANG_STRUCTURED_BUFFER,    SLANG_RESOURCE_ACCESS_READ);
    CASE(HLSLRWStructuredBufferType,                    SLANG_STRUCTURED_BUFFER,    SLANG_RESOURCE_ACCESS_READ_WRITE);
    CASE(HLSLRasterizerOrderedStructuredBufferType,     SLANG_STRUCTURED_BUFFER,    SLANG_RESOURCE_ACCESS_RASTER_ORDERED);
    CASE(HLSLAppendStructuredBufferType,                SLANG_STRUCTURED_BUFFER,    SLANG_RESOURCE_ACCESS_APPEND);
    CASE(HLSLConsumeStructuredBufferType,               SLANG_STRUCTURED_BUFFER,    SLANG_RESOURCE_ACCESS_CONSUME);
    CASE(HLSLByteAddressBufferType,                     SLANG_BYTE_ADDRESS_BUFFER,  SLANG_RESOURCE_ACCESS_READ);
    CASE(HLSLRWByteAddressBufferType,                   SLANG_BYTE_ADDRESS_BUFFER,  SLANG_RESOURCE_ACCESS_READ_WRITE);
    CASE(HLSLRasterizerOrderedByteAddressBufferType,    SLANG_BYTE_ADDRESS_BUFFER,  SLANG_RESOURCE_ACCESS_RASTER_ORDERED);
    CASE(UntypedBufferResourceType,                     SLANG_BYTE_ADDRESS_BUFFER,  SLANG_RESOURCE_ACCESS_READ);

    // This isn't entirely accurate, but I can live with it for now
    CASE(GLSLShaderStorageBufferType, SLANG_STRUCTURED_BUFFER, SLANG_RESOURCE_ACCESS_READ_WRITE);
#undef CASE

    return SLANG_RESOURCE_ACCESS_NONE;
}

char const* spReflectionType_GetName(SlangReflectionType* inType)
{
    using namespace Slang;
    using namespace Reflection;

    auto type = convert(inType);

    if( auto declRefType = as<DeclRefType>(type) )
    {
        auto declRef = declRefType->declRef;

        // Don't return a name for auto-generated anonymous types
        // that represent `cbuffer` members, etc.
        auto decl = declRef.getDecl();
        if(decl->hasModifier<ImplicitParameterGroupElementTypeModifier>())
            return nullptr;

        return getText(declRef.getName()).begin();
    }

    return nullptr;
}

SlangReflectionType * spReflection_FindTypeByName(SlangReflection * reflection, char const * name)
{
    using namespace Slang;
    using namespace Reflection;

    auto programLayout = convert(reflection);
    auto program = programLayout->getProgram();

    // TODO: We should extend this API to support getting error messages
    // when type lookup fails.
    //
    Slang::DiagnosticSink sink(
        programLayout->getTargetReq()->getLinkage()->getSourceManager());

    try
    {
        Type* result = program->getTypeFromString(name, &sink);
        return (SlangReflectionType*)result;
    }
    catch( ... )
    {
        return nullptr;
    }
}

SlangReflectionTypeLayout* spReflection_GetTypeLayout(
    SlangReflection* reflection,
    SlangReflectionType* inType,
    SlangLayoutRules /*rules*/)
{
    using namespace Slang;
    using namespace Reflection;

    auto context = convert(reflection);
    auto type = convert(inType);
    auto targetReq = context->getTargetReq();

    auto typeLayout = targetReq->getTypeLayout(type);
    return convert(typeLayout);
}

SlangReflectionType* spReflectionType_GetResourceResultType(SlangReflectionType* inType)
{
    using namespace Slang;
    using namespace Reflection;

    auto type = convert(inType);
    if(!type) return nullptr;

    while(auto arrayType = as<ArrayExpressionType>(type))
    {
        type = arrayType->baseType;
    }

    if (auto textureType = as<TextureTypeBase>(type))
    {
        return convert(textureType->elementType);
    }

    // TODO: need a better way to handle this stuff...
#define CASE(TYPE, SHAPE, ACCESS)                                                       \
    else if(as<TYPE>(type)) do {                                                      \
        return convert(as<TYPE>(type)->elementType);                            \
    } while(0)

    // TODO: structured buffer needs to expose type layout!

    CASE(HLSLStructuredBufferType,                  SLANG_STRUCTURED_BUFFER, SLANG_RESOURCE_ACCESS_READ);
    CASE(HLSLRWStructuredBufferType,                SLANG_STRUCTURED_BUFFER, SLANG_RESOURCE_ACCESS_READ_WRITE);
    CASE(HLSLRasterizerOrderedStructuredBufferType, SLANG_STRUCTURED_BUFFER, SLANG_RESOURCE_ACCESS_RASTER_ORDERED);
    CASE(HLSLAppendStructuredBufferType,            SLANG_STRUCTURED_BUFFER, SLANG_RESOURCE_ACCESS_APPEND);
    CASE(HLSLConsumeStructuredBufferType,           SLANG_STRUCTURED_BUFFER, SLANG_RESOURCE_ACCESS_CONSUME);
#undef CASE

    return nullptr;
}

// type Layout Reflection

SlangReflectionType* spReflectionTypeLayout_GetType(SlangReflectionTypeLayout* inTypeLayout)
{
    using namespace Slang;
    using namespace Reflection;

    auto typeLayout = convert(inTypeLayout);
    if(!typeLayout) return nullptr;

    return (SlangReflectionType*) typeLayout->type;
}

size_t spReflectionTypeLayout_GetSize(SlangReflectionTypeLayout* inTypeLayout, SlangParameterCategory category)
{
    using namespace Slang;
    using namespace Reflection;

    auto typeLayout = convert(inTypeLayout);
    if(!typeLayout) return 0;

    auto info = typeLayout->FindResourceInfo(LayoutResourceKind(category));
    if(!info) return 0;

    return getReflectionSize(info->count);
}

int32_t spReflectionTypeLayout_getAlignment(SlangReflectionTypeLayout* inTypeLayout, SlangParameterCategory category)
{
    using namespace Slang;
    using namespace Reflection;

    auto typeLayout = convert(inTypeLayout);
    if(!typeLayout) return 0;

    if( category == SLANG_PARAMETER_CATEGORY_UNIFORM )
    {
        return int32_t(typeLayout->uniformAlignment);
    }
    else
    {
        return 1;
    }
}

SlangReflectionVariableLayout* spReflectionTypeLayout_GetFieldByIndex(SlangReflectionTypeLayout* inTypeLayout, unsigned index)
{
    using namespace Slang;
    using namespace Reflection;

    auto typeLayout = convert(inTypeLayout);
    if(!typeLayout) return nullptr;

    if(auto structTypeLayout = as<StructTypeLayout>(typeLayout))
    {
        return (SlangReflectionVariableLayout*) structTypeLayout->fields[index].Ptr();
    }

    return nullptr;
}

size_t spReflectionTypeLayout_GetElementStride(SlangReflectionTypeLayout* inTypeLayout, SlangParameterCategory category)
{
    using namespace Slang;
    using namespace Reflection;

    auto typeLayout = convert(inTypeLayout);
    if(!typeLayout) return 0;

    if( auto arrayTypeLayout = as<ArrayTypeLayout>(typeLayout))
    {
        switch (category)
        {
        // We store the stride explicitly for the uniform case
        case SLANG_PARAMETER_CATEGORY_UNIFORM:
            return arrayTypeLayout->uniformStride;

        // For most other cases (resource registers), the "stride"
        // of an array is simply the number of resources (if any)
        // consumed by its element type.
        default:
            {
                auto elementTypeLayout = arrayTypeLayout->elementTypeLayout;
                auto info = elementTypeLayout->FindResourceInfo(LayoutResourceKind(category));
                if(!info) return 0;
                return getReflectionSize(info->count);
            }

        // An important special case, though, is Vulkan descriptor-table slots,
        // where an entire array will use a single `binding`, so that the
        // effective stride is zero:
        case SLANG_PARAMETER_CATEGORY_DESCRIPTOR_TABLE_SLOT:
            return 0;
        }
    }

    return 0;
}

SlangReflectionTypeLayout* spReflectionTypeLayout_GetElementTypeLayout(SlangReflectionTypeLayout* inTypeLayout)
{
    using namespace Slang;
    using namespace Reflection;

    auto typeLayout = convert(inTypeLayout);
    if(!typeLayout) return nullptr;

    if( auto arrayTypeLayout = as<ArrayTypeLayout>(typeLayout))
    {
        return (SlangReflectionTypeLayout*) arrayTypeLayout->elementTypeLayout.Ptr();
    }
    else if( auto constantBufferTypeLayout = as<ParameterGroupTypeLayout>(typeLayout))
    {
        return convert(constantBufferTypeLayout->offsetElementTypeLayout.Ptr());
    }
    else if( auto structuredBufferTypeLayout = as<StructuredBufferTypeLayout>(typeLayout))
    {
        return convert(structuredBufferTypeLayout->elementTypeLayout.Ptr());
    }
    else if( auto specializedTypeLayout = as<ExistentialSpecializedTypeLayout>(typeLayout) )
    {
        return convert(specializedTypeLayout->baseTypeLayout.Ptr());
    }

    return nullptr;
}

SlangReflectionVariableLayout* spReflectionTypeLayout_GetElementVarLayout(SlangReflectionTypeLayout* inTypeLayout)
{
    using namespace Slang;
    using namespace Reflection;

    auto typeLayout = convert(inTypeLayout);
    if(!typeLayout) return nullptr;

    if( auto parameterGroupTypeLayout = as<ParameterGroupTypeLayout>(typeLayout))
    {
        return convert(parameterGroupTypeLayout->elementVarLayout.Ptr());
    }

    return nullptr;
}

SlangReflectionVariableLayout* spReflectionTypeLayout_getContainerVarLayout(SlangReflectionTypeLayout* inTypeLayout)
{
    using namespace Slang;
    using namespace Reflection;

    auto typeLayout = convert(inTypeLayout);
    if(!typeLayout) return nullptr;

    if( auto parameterGroupTypeLayout = as<ParameterGroupTypeLayout>(typeLayout))
    {
        return convert(parameterGroupTypeLayout->containerVarLayout.Ptr());
    }

    return nullptr;
}

SlangParameterCategory spReflectionTypeLayout_GetParameterCategory(SlangReflectionTypeLayout* inTypeLayout)
{
    using namespace Slang;
    using namespace Reflection;

    auto typeLayout = convert(inTypeLayout);
    if(!typeLayout) return SLANG_PARAMETER_CATEGORY_NONE;

    typeLayout = maybeGetContainerLayout(typeLayout);

    return getParameterCategory(typeLayout);
}

unsigned spReflectionTypeLayout_GetCategoryCount(SlangReflectionTypeLayout* inTypeLayout)
{
    using namespace Slang;
    using namespace Reflection;

    auto typeLayout = convert(inTypeLayout);
    if(!typeLayout) return 0;

    typeLayout = maybeGetContainerLayout(typeLayout);

    return (unsigned) typeLayout->resourceInfos.getCount();
}

SlangParameterCategory spReflectionTypeLayout_GetCategoryByIndex(SlangReflectionTypeLayout* inTypeLayout, unsigned index)
{
    using namespace Slang;
    using namespace Reflection;

    auto typeLayout = convert(inTypeLayout);
    if(!typeLayout) return SLANG_PARAMETER_CATEGORY_NONE;

    typeLayout = maybeGetContainerLayout(typeLayout);

    return typeLayout->resourceInfos[index].kind;
}

SlangMatrixLayoutMode spReflectionTypeLayout_GetMatrixLayoutMode(SlangReflectionTypeLayout* inTypeLayout)
{
    using namespace Slang;
    using namespace Reflection;

    auto typeLayout = convert(inTypeLayout);
    if(!typeLayout) return SLANG_MATRIX_LAYOUT_MODE_UNKNOWN;

    if( auto matrixLayout = as<MatrixTypeLayout>(typeLayout) )
    {
        return matrixLayout->mode;
    }
    else
    {
        return SLANG_MATRIX_LAYOUT_MODE_UNKNOWN;
    }

}

int spReflectionTypeLayout_getGenericParamIndex(SlangReflectionTypeLayout* inTypeLayout)
{
    using namespace Slang;
    using namespace Reflection;

    auto typeLayout = convert(inTypeLayout);
    if(!typeLayout) return -1;

    if(auto genericParamTypeLayout = as<GenericParamTypeLayout>(typeLayout))
    {
        return (int) genericParamTypeLayout->paramIndex;
    }
    else
    {
        return -1;
    }
}

SlangReflectionTypeLayout* spReflectionTypeLayout_getPendingDataTypeLayout(SlangReflectionTypeLayout* inTypeLayout)
{
    using namespace Slang;
    using namespace Reflection;

    auto typeLayout = convert(inTypeLayout);
    if(!typeLayout) return nullptr;

    auto pendingDataTypeLayout = typeLayout->pendingDataTypeLayout.Ptr();
    return convert(pendingDataTypeLayout);
}

SlangReflectionVariableLayout* spReflectionVariableLayout_getPendingDataLayout(SlangReflectionVariableLayout* inVarLayout)
{
    using namespace Slang;
    using namespace Reflection;

    auto varLayout = convert(inVarLayout);
    if(!varLayout) return nullptr;

    auto pendingDataLayout = varLayout->pendingVarLayout.Ptr();
    return convert(pendingDataLayout);
}

SlangReflectionVariableLayout* spReflectionTypeLayout_getSpecializedTypePendingDataVarLayout(SlangReflectionTypeLayout* inTypeLayout)
{
    using namespace Slang;
    using namespace Reflection;

    auto typeLayout = convert(inTypeLayout);
    if(!typeLayout) return nullptr;

    if( auto specializedTypeLayout = as<ExistentialSpecializedTypeLayout>(typeLayout) )
    {
        auto pendingDataVarLayout = specializedTypeLayout->pendingDataVarLayout.Ptr();
        return convert(pendingDataVarLayout);
    }
    else
    {
        return nullptr;
    }
}


// Variable Reflection

char const* spReflectionVariable_GetName(SlangReflectionVariable* inVar)
{
    using namespace Slang;
    using namespace Reflection;

    auto var = convert(inVar);
    if(!var) return nullptr;

    // If the variable is one that has an "external" name that is supposed
    // to be exposed for reflection, then report it here
    if(auto reflectionNameMod = var->findModifier<ParameterGroupReflectionName>())
        return getText(reflectionNameMod->nameAndLoc.name).getBuffer();

    return getText(var->getName()).getBuffer();
}

SlangReflectionType* spReflectionVariable_GetType(SlangReflectionVariable* inVar)
{
    using namespace Slang;
    using namespace Reflection;

    auto var = convert(inVar);
    if(!var) return nullptr;

    return  convert(var->getType());
}

SlangReflectionModifier* spReflectionVariable_FindModifier(SlangReflectionVariable* inVar, SlangModifierID modifierID)
{
    using namespace Slang;
    using namespace Reflection;

    auto var = convert(inVar);
    if(!var) return nullptr;

    Modifier* modifier = nullptr;
    switch( modifierID )
    {
    case SLANG_MODIFIER_SHARED:
        modifier = var->findModifier<HLSLEffectSharedModifier>();
        break;
    case SLANG_MODIFIER_PUSH_CONSTANT:
        modifier = var->findModifier<PushConstantAttribute>();
        break;

    default:
        return nullptr;
    }

    return (SlangReflectionModifier*) modifier;
}

unsigned int spReflectionVariable_GetUserAttributeCount(SlangReflectionVariable* inVar)
{
    using namespace Slang;
    using namespace Reflection;

    auto varDecl = convert(inVar);
    if (!varDecl) return 0;
    return getUserAttributeCount(varDecl);
}
SlangReflectionUserAttribute* spReflectionVariable_GetUserAttribute(SlangReflectionVariable* inVar, unsigned int index)
{
    using namespace Slang;
    using namespace Reflection;

    auto varDecl = convert(inVar);
    if (!varDecl) return 0;
    return getUserAttributeByIndex(varDecl, index);
}
SlangReflectionUserAttribute* spReflectionVariable_FindUserAttributeByName(SlangReflectionVariable* inVar, SlangSession* session, char const* name)
{
    using namespace Slang;
    using namespace Reflection;

    auto varDecl = convert(inVar);
    if (!varDecl) return 0;
    return findUserAttributeByName(Slang::Reflection::convert(session), varDecl, name);
}

// Variable Layout Reflection

SlangReflectionVariable* spReflectionVariableLayout_GetVariable(SlangReflectionVariableLayout* inVarLayout)
{
    using namespace Slang;
    using namespace Reflection;

    auto varLayout = convert(inVarLayout);
    if(!varLayout) return nullptr;

    return convert(varLayout->varDecl.getDecl());
}

SlangReflectionTypeLayout* spReflectionVariableLayout_GetTypeLayout(SlangReflectionVariableLayout* inVarLayout)
{
    using namespace Slang;
    using namespace Reflection;

    auto varLayout = convert(inVarLayout);
    if(!varLayout) return nullptr;

    return convert(varLayout->getTypeLayout());
}

namespace Slang
{
    // Attempt "do what I mean" remapping from the parameter category the user asked about,
    // over to a parameter category that they might have meant.
    static SlangParameterCategory maybeRemapParameterCategory(
        TypeLayout*             typeLayout,
        SlangParameterCategory  category)
    {
        using namespace Reflection;

        // Do we have an entry for the category they asked about? Then use that.
        if (typeLayout->FindResourceInfo(LayoutResourceKind(category)))
            return category;

        // Do we have an entry for the `DescriptorTableSlot` category?
        if (typeLayout->FindResourceInfo(LayoutResourceKind::DescriptorTableSlot))
        {
            // Is the category they were asking about one that makes sense for the type
            // of this variable?
            Type* type = typeLayout->getType();
            while (auto arrayType = as<ArrayExpressionType>(type))
                type = arrayType->baseType;
            switch (spReflectionType_GetKind(convert(type)))
            {
            case SLANG_TYPE_KIND_CONSTANT_BUFFER:
                if(category == SLANG_PARAMETER_CATEGORY_CONSTANT_BUFFER)
                    return SLANG_PARAMETER_CATEGORY_DESCRIPTOR_TABLE_SLOT;
                break;

            case SLANG_TYPE_KIND_RESOURCE:
                if(category == SLANG_PARAMETER_CATEGORY_SHADER_RESOURCE)
                    return SLANG_PARAMETER_CATEGORY_DESCRIPTOR_TABLE_SLOT;
                break;

            case SLANG_TYPE_KIND_SAMPLER_STATE:
                if(category == SLANG_PARAMETER_CATEGORY_SAMPLER_STATE)
                    return SLANG_PARAMETER_CATEGORY_DESCRIPTOR_TABLE_SLOT;
                break;

            // TODO: implement more helpers here

            default:
                break;
            }
        }

        return category;
    }
}

size_t spReflectionVariableLayout_GetOffset(SlangReflectionVariableLayout* inVarLayout, SlangParameterCategory category)
{
    using namespace Slang;
    using namespace Reflection;

    auto varLayout = convert(inVarLayout);
    if(!varLayout) return 0;

    auto info = varLayout->FindResourceInfo(LayoutResourceKind(category));

    if (!info)
    {
        // No match with requested category? Try again with one they might have meant...
        category = maybeRemapParameterCategory(varLayout->getTypeLayout(), category);
        info = varLayout->FindResourceInfo(LayoutResourceKind(category));
    }

    if(!info) return 0;

    return info->index;
}

size_t spReflectionVariableLayout_GetSpace(SlangReflectionVariableLayout* inVarLayout, SlangParameterCategory category)
{
    using namespace Slang;
    using namespace Reflection;

    auto varLayout = convert(inVarLayout);
    if(!varLayout) return 0;


    auto info = varLayout->FindResourceInfo(LayoutResourceKind(category));
    if (!info)
    {
        // No match with requested category? Try again with one they might have meant...
        category = maybeRemapParameterCategory(varLayout->getTypeLayout(), category);
        info = varLayout->FindResourceInfo(LayoutResourceKind(category));
    }

    UInt space = 0;

    // First, deal with any offset applied to the specific resource kind specified
    if (info)
    {
        space += info->space;
    }

    // Note: this code used to try and take a variable with
    // an offset for `LayoutResourceKind::RegisterSpace` and
    // add it to the space returned, but that isn't going
    // to be right in some cases.
    //
    // Imageine if we have:
    //
    //  struct X { Texture2D y; }
    //  struct S { Texture2D t; ParmaeterBlock<X> x; }
    //
    //  Texture2D gA;
    //  S gS;
    //
    // We expect `gS` to have an offset for `LayoutResourceKind::ShaderResourceView`
    // of one (since its texture must come after `gA`), and an offset for
    // `LayoutResourceKind::RegisterSpace` of one (since the default space will be
    // space zero). It would be incorrect for us to imply that `gS.t` should
    // be `t1, space1`, though, because the space offset of `gS` doesn't actually
    // apply to `t`.
    //
    // For now we are punting on this issue and leaving it in the hands of the
    // application to determine when a space offset from an "outer" variable should
    // apply to the locations of things in an "inner" variable.
    //
    // There is no policy we can apply locally in this function that
    // will Just Work, so the best we can do is try to not lie.

    return space;
}

char const* spReflectionVariableLayout_GetSemanticName(SlangReflectionVariableLayout* inVarLayout)
{
    using namespace Slang;
    using namespace Reflection;

    auto varLayout = convert(inVarLayout);
    if(!varLayout) return 0;

    if (!(varLayout->flags & Slang::VarLayoutFlag::HasSemantic))
        return 0;

    return varLayout->semanticName.getBuffer();
}

size_t spReflectionVariableLayout_GetSemanticIndex(SlangReflectionVariableLayout* inVarLayout)
{
    using namespace Slang;
    using namespace Reflection;

    auto varLayout = convert(inVarLayout);
    if(!varLayout) return 0;

    if (!(varLayout->flags & Slang::VarLayoutFlag::HasSemantic))
        return 0;

    return varLayout->semanticIndex;
}

SlangStage spReflectionVariableLayout_getStage(
    SlangReflectionVariableLayout* inVarLayout)
{
    using namespace Slang;
    using namespace Reflection;

    auto varLayout = convert(inVarLayout);
    if(!varLayout) return SLANG_STAGE_NONE;

    // A parameter that is not a varying input or output is
    // not considered to belong to a single stage.
    //
    // TODO: We might need to reconsider this for, e.g., entry
    // point parameters, where they might be stage-specific even
    // if they are uniform.
    if (!varLayout->FindResourceInfo(Slang::LayoutResourceKind::VaryingInput)
        && !varLayout->FindResourceInfo(Slang::LayoutResourceKind::VaryingOutput))
    {
        return SLANG_STAGE_NONE;
    }

    // TODO: We should find the stage for a variable layout by
    // walking up the tree of layout information, until we find
    // something that has a definitive stage attached to it (e.g.,
    // either an entry point or a GLSL translation unit).
    //
    // We don't currently have parent links in the reflection layout
    // information, so doing that walk would be tricky right now, so
    // it is easier to just bloat the representation and store yet another
    // field on every variable layout.
    return (SlangStage) varLayout->stage;
}


// Shader Parameter Reflection

unsigned spReflectionParameter_GetBindingIndex(SlangReflectionParameter* inVarLayout)
{
    SlangReflectionVariableLayout* varLayout = (SlangReflectionVariableLayout*)inVarLayout;
    return (unsigned) spReflectionVariableLayout_GetOffset(
        varLayout,
        spReflectionTypeLayout_GetParameterCategory(
            spReflectionVariableLayout_GetTypeLayout(varLayout)));
}

unsigned spReflectionParameter_GetBindingSpace(SlangReflectionParameter* inVarLayout)
{
    SlangReflectionVariableLayout* varLayout = (SlangReflectionVariableLayout*)inVarLayout;
    return (unsigned) spReflectionVariableLayout_GetSpace(
        varLayout,
        spReflectionTypeLayout_GetParameterCategory(
            spReflectionVariableLayout_GetTypeLayout(varLayout)));
}

// Helpers for getting parameter count

namespace Slang
{
    static unsigned getParameterCount(RefPtr<TypeLayout> typeLayout)
    {
        if(auto parameterGroupLayout = as<ParameterGroupTypeLayout>(typeLayout))
        {
            typeLayout = parameterGroupLayout->offsetElementTypeLayout;
        }

        if(auto structLayout = as<StructTypeLayout>(typeLayout))
        {
            return (unsigned) structLayout->fields.getCount();
        }

        return 0;
    }

    static VarLayout* getParameterByIndex(RefPtr<TypeLayout> typeLayout, unsigned index)
    {
        if(auto parameterGroupLayout = as<ParameterGroupTypeLayout>(typeLayout))
        {
            typeLayout = parameterGroupLayout->offsetElementTypeLayout;
        }

        if(auto structLayout = as<StructTypeLayout>(typeLayout))
        {
            return structLayout->fields[index];
        }

        return 0;
    }
}

// Entry Point Reflection

char const* spReflectionEntryPoint_getName(
    SlangReflectionEntryPoint* inEntryPoint)
{
    using namespace Slang;
    using namespace Reflection;

    auto entryPointLayout = convert(inEntryPoint);
    return entryPointLayout ? getCstr(entryPointLayout->name) : nullptr;
}

unsigned spReflectionEntryPoint_getParameterCount(
    SlangReflectionEntryPoint* inEntryPoint)
{
    using namespace Slang;
    using namespace Reflection;

    auto entryPointLayout = convert(inEntryPoint);
    if(!entryPointLayout) return 0;

    return getParameterCount(entryPointLayout->parametersLayout->typeLayout);
}

SlangReflectionVariableLayout* spReflectionEntryPoint_getParameterByIndex(
    SlangReflectionEntryPoint*  inEntryPoint,
    unsigned                    index)
{
    using namespace Slang;
    using namespace Reflection;

    auto entryPointLayout = convert(inEntryPoint);
    if(!entryPointLayout) return 0;

    return convert(getParameterByIndex(entryPointLayout->parametersLayout->typeLayout, index));
}

SlangStage spReflectionEntryPoint_getStage(SlangReflectionEntryPoint* inEntryPoint)
{
    using namespace Slang;
    using namespace Reflection;

    auto entryPointLayout = convert(inEntryPoint);

    if(!entryPointLayout) return SLANG_STAGE_NONE;

    return SlangStage(entryPointLayout->profile.getStage());
}

void spReflectionEntryPoint_getComputeThreadGroupSize(
    SlangReflectionEntryPoint*  inEntryPoint,
    SlangUInt                   axisCount,
    SlangUInt*                  outSizeAlongAxis)
{
    using namespace Slang;
    using namespace Reflection;

    auto entryPointLayout = convert(inEntryPoint);

    if(!entryPointLayout)   return;
    if(!axisCount)          return;
    if(!outSizeAlongAxis)   return;

    auto entryPointFunc = entryPointLayout->entryPoint;
    if(!entryPointFunc) return;

    SlangUInt sizeAlongAxis[3] = { 1, 1, 1 };

    // First look for the HLSL case, where we have an attribute attached to the entry point function
    auto numThreadsAttribute = entryPointFunc.getDecl()->findModifier<NumThreadsAttribute>();
    if (numThreadsAttribute)
    {
        sizeAlongAxis[0] = numThreadsAttribute->x;
        sizeAlongAxis[1] = numThreadsAttribute->y;
        sizeAlongAxis[2] = numThreadsAttribute->z;
    }

    //

    if(axisCount > 0) outSizeAlongAxis[0] = sizeAlongAxis[0];
    if(axisCount > 1) outSizeAlongAxis[1] = sizeAlongAxis[1];
    if(axisCount > 2) outSizeAlongAxis[2] = sizeAlongAxis[2];
    for( SlangUInt aa = 3; aa < axisCount; ++aa )
    {
        outSizeAlongAxis[aa] = 1;
    }
}

int spReflectionEntryPoint_usesAnySampleRateInput(
    SlangReflectionEntryPoint* inEntryPoint)
{
    using namespace Slang;
    using namespace Reflection;

    auto entryPointLayout = convert(inEntryPoint);
    if(!entryPointLayout)
        return 0;

    if (entryPointLayout->profile.getStage() != Stage::Fragment)
        return 0;

    return (entryPointLayout->flags & EntryPointLayout::Flag::usesAnySampleRateInput) != 0;
}

SlangReflectionVariableLayout* spReflectionEntryPoint_getVarLayout(
    SlangReflectionEntryPoint* inEntryPoint)
{
    using namespace Slang;
    using namespace Reflection;

    auto entryPointLayout = convert(inEntryPoint);
    if(!entryPointLayout)
        return nullptr;

    return convert(entryPointLayout->parametersLayout);
}

SlangReflectionVariableLayout* spReflectionEntryPoint_getResultVarLayout(
    SlangReflectionEntryPoint* inEntryPoint)
{
    using namespace Slang;
    using namespace Reflection;

    auto entryPointLayout = convert(inEntryPoint);
    if(!entryPointLayout)
        return nullptr;

    return convert(entryPointLayout->resultLayout);
}

int spReflectionEntryPoint_hasDefaultConstantBuffer(
    SlangReflectionEntryPoint* inEntryPoint)
{
    using namespace Slang;
    using namespace Reflection;

    auto entryPointLayout = convert(inEntryPoint);
    if(!entryPointLayout)
        return 0;

    return hasDefaultConstantBuffer(entryPointLayout);
}


// SlangReflectionTypeParameter
char const* spReflectionTypeParameter_GetName(SlangReflectionTypeParameter * inTypeParam)
{
    using namespace Slang;
    using namespace Reflection;
    
    auto specializationParam = convert(inTypeParam);
    if( auto genericParamLayout = as<GenericSpecializationParamLayout>(specializationParam) )
    {
        return genericParamLayout->decl->getName()->text.getBuffer();
    }
    // TODO: Add case for existential type parameter? They don't have as simple of a notion of "name" as the generic case...
    return nullptr;
}

unsigned spReflectionTypeParameter_GetIndex(SlangReflectionTypeParameter * inTypeParam)
{
    using namespace Slang;
    using namespace Reflection;

    auto typeParam = convert(inTypeParam);
    return (unsigned)(typeParam->index);
}

unsigned int spReflectionTypeParameter_GetConstraintCount(SlangReflectionTypeParameter* inTypeParam)
{
    using namespace Slang;
    using namespace Reflection;

    auto specializationParam = convert(inTypeParam);
    if(auto genericParamLayout = as<GenericSpecializationParamLayout>(specializationParam))
    {
        if( auto globalGenericParamDecl = as<GlobalGenericParamDecl>(genericParamLayout->decl) )
        {
            auto constraints = globalGenericParamDecl->getMembersOfType<GenericTypeConstraintDecl>();
            return (unsigned int)constraints.getCount();
        }
        // TODO: Add case for entry-point generic parameters.
    }
    // TODO: Add case for existential type parameters.
    return 0;
}

SlangReflectionType* spReflectionTypeParameter_GetConstraintByIndex(SlangReflectionTypeParameter * inTypeParam, unsigned index)
{
    using namespace Slang;
    using namespace Reflection;
    
    auto specializationParam = convert(inTypeParam);
    if(auto genericParamLayout = as<GenericSpecializationParamLayout>(specializationParam))
    {
        if( auto globalGenericParamDecl = as<GlobalGenericParamDecl>(genericParamLayout->decl) )
        {
            auto constraints = globalGenericParamDecl->getMembersOfType<GenericTypeConstraintDecl>();
            return (SlangReflectionType*)constraints[index]->sup.Ptr();
        }
        // TODO: Add case for entry-point generic parameters.
    }
    // TODO: Add case for existential type parameters.
    return 0;
}

// Shader Reflection

unsigned spReflection_GetParameterCount(SlangReflection* inProgram)
{
    using namespace Slang;
    using namespace Reflection;

    auto program = convert(inProgram);
    if(!program) return 0;

    auto globalStructLayout = getGlobalStructLayout(program);
    if (!globalStructLayout)
        return 0;

    return (unsigned) globalStructLayout->fields.getCount();
}

SlangReflectionParameter* spReflection_GetParameterByIndex(SlangReflection* inProgram, unsigned index)
{
    using namespace Slang;
    using namespace Reflection;

    auto program = convert(inProgram);
    if(!program) return nullptr;

    auto globalStructLayout = getGlobalStructLayout(program);
    if (!globalStructLayout)
        return 0;

    return convert(globalStructLayout->fields[index].Ptr());
}

unsigned int spReflection_GetTypeParameterCount(SlangReflection * reflection)
{
    using namespace Slang;
    using namespace Reflection;

    auto program = convert(reflection);
    return (unsigned int) program->specializationParams.getCount();
}

SlangReflectionTypeParameter* spReflection_GetTypeParameterByIndex(SlangReflection * reflection, unsigned int index)
{
    using namespace Slang;
    using namespace Reflection;

    auto program = convert(reflection);
    return (SlangReflectionTypeParameter*) program->specializationParams[index].Ptr();
}

SlangReflectionTypeParameter * spReflection_FindTypeParameter(SlangReflection * inProgram, char const * name)
{
    using namespace Slang;
    using namespace Reflection;

    auto program = convert(inProgram);
    if (!program) return nullptr;
    for( auto& param : program->specializationParams )
    {
        auto genericParamLayout = as<GenericSpecializationParamLayout>(param);
        if(!genericParamLayout)
            continue;

        if(getText(genericParamLayout->decl->getName()) != UnownedTerminatedStringSlice(name))
            continue;

        return (SlangReflectionTypeParameter*) genericParamLayout;
    }

    return 0;
}

SlangUInt spReflection_getEntryPointCount(SlangReflection* inProgram)
{
    using namespace Slang;
    using namespace Reflection;

    auto program = convert(inProgram);
    if(!program) return 0;

    return SlangUInt(program->entryPoints.getCount());
}

SlangReflectionEntryPoint* spReflection_getEntryPointByIndex(SlangReflection* inProgram, SlangUInt index)
{
    using namespace Slang;
    using namespace Reflection;

    auto program = convert(inProgram);
    if(!program) return 0;

    return convert(program->entryPoints[(int) index].Ptr());
}

SlangReflectionEntryPoint* spReflection_findEntryPointByName(SlangReflection* inProgram, char const* name)
{
    using namespace Slang;
    using namespace Reflection;

    auto program = convert(inProgram);
    if(!program) return 0;

    // TODO: improve on naive linear search
    for(auto ep : program->entryPoints)
    {
        if(ep->entryPoint.getName()->text == name)
        {
            return convert(ep);
        }
    }

    return nullptr;
}

SlangUInt spReflection_getGlobalConstantBufferBinding(SlangReflection* inProgram)
{
    using namespace Slang;
    using namespace Reflection;

    auto program = convert(inProgram);
    if (!program) return 0;
    auto cb = program->parametersLayout->FindResourceInfo(LayoutResourceKind::ConstantBuffer);
    if (!cb) return 0;
    return cb->index;
}

size_t spReflection_getGlobalConstantBufferSize(SlangReflection* inProgram)
{
    using namespace Slang;
    using namespace Reflection;

    auto program = convert(inProgram);
    if (!program) return 0;
    auto structLayout = getGlobalStructLayout(program);
    auto uniform = structLayout->FindResourceInfo(LayoutResourceKind::Uniform);
    if (!uniform) return 0;
    return getReflectionSize(uniform->count);
}

 SlangReflectionType* spReflection_specializeType(
    SlangReflection*            inProgramLayout,
    SlangReflectionType*        inType,
    SlangInt                    specializationArgCount,
    SlangReflectionType* const* specializationArgs,
    ISlangBlob**                outDiagnostics)
{
    using namespace Slang;
    using namespace Reflection;
    
    auto programLayout = convert(inProgramLayout);
    if(!programLayout) return nullptr;

    auto unspecializedType = convert(inType);
    if(!unspecializedType) return nullptr;

    auto linkage = programLayout->getProgram()->getLinkage();

    DiagnosticSink sink(linkage->getSourceManager());

    auto specializedType = linkage->specializeType(unspecializedType, specializationArgCount, (Type* const*) specializationArgs, &sink);

    sink.getBlobIfNeeded(outDiagnostics);

    return convert(specializedType);
}

SlangUInt spReflection_getHashedStringCount(
    SlangReflection*  reflection)
{
    using namespace Slang;
    using namespace Reflection;

    auto programLayout = convert(reflection);
    auto slices = programLayout->hashedStringLiteralPool.getAdded();
    return slices.getCount();
}

const char* spReflection_getHashedString(
    SlangReflection*  reflection,
    SlangUInt index,
    size_t* outCount)
{
    using namespace Slang;
    using namespace Reflection;
    
    auto programLayout = convert(reflection);

    auto slices = programLayout->hashedStringLiteralPool.getAdded();
    auto slice = slices[Index(index)];

    *outCount = slice.getLength();
    return slice.begin();
}

int spComputeStringHash(const char* chars, size_t count)
{
    using namespace Slang;

    return (int)getStableHashCode32(chars, count);
}
