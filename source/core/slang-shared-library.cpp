#include "slang-shared-library.h"

#include "../../slang-com-ptr.h"

#include "slang-io.h"
#include "slang-string-util.h"

namespace Slang
{

namespace SlangSharedLibrary {
// Allocate static const storage for the various interface IDs that the Slang API needs to expose
static const Guid IID_ISlangUnknown = SLANG_UUID_ISlangUnknown;
static const Guid IID_ISlangSharedLibrary = SLANG_UUID_ISlangSharedLibrary;
static const Guid IID_ISlangSharedLibraryLoader = SLANG_UUID_ISlangSharedLibraryLoader;
}

/* !!!!!!!!!!!!!!!!!!!!!!!!!! DefaultSharedLibraryLoader !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!*/

/* static */DefaultSharedLibraryLoader DefaultSharedLibraryLoader::s_singleton;

ISlangUnknown* DefaultSharedLibraryLoader::getInterface(const Guid& guid)
{
    return (guid == SlangSharedLibrary::IID_ISlangUnknown || guid == SlangSharedLibrary::IID_ISlangSharedLibraryLoader) ? static_cast<ISlangSharedLibraryLoader*>(this) : nullptr;
}

SlangResult DefaultSharedLibraryLoader::loadSharedLibrary(const char* path, ISlangSharedLibrary** outSharedLibrary)
{
    *outSharedLibrary = nullptr;
    // Try loading
    SharedLibrary::Handle handle;
    SLANG_RETURN_ON_FAIL(SharedLibrary::load(path, handle));
    *outSharedLibrary = ComPtr<ISlangSharedLibrary>(new DefaultSharedLibrary(handle)).detach();
    return SLANG_OK;
}

SlangResult DefaultSharedLibraryLoader::loadPlatformSharedLibrary(const char* path, ISlangSharedLibrary** outSharedLibrary)
{
    *outSharedLibrary = nullptr;
    // Try loading
    SharedLibrary::Handle handle;
    SLANG_RETURN_ON_FAIL(SharedLibrary::loadWithPlatformPath(path, handle));
    *outSharedLibrary = ComPtr<ISlangSharedLibrary>(new DefaultSharedLibrary(handle)).detach();
    return SLANG_OK;
}

/* static */SlangResult DefaultSharedLibraryLoader::load(ISlangSharedLibraryLoader* loader, const String& path, const String& name, ISlangSharedLibrary** outLibrary)
{
    if (path.getLength())
    {
        String combinedPath = Path::combine(path, name);
        return loader->loadSharedLibrary(combinedPath.getBuffer(), outLibrary);
    }
    else
    {
        return loader->loadSharedLibrary(name.getBuffer(), outLibrary);
    }
}

/* !!!!!!!!!!!!!!!!!!!!!!!!!! DefaultSharedLibrary !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!*/

TemporarySharedLibrary::~TemporarySharedLibrary()
{
    if (m_sharedLibraryHandle)
    {
        // We have to unload if we want to be able to remove
        SharedLibrary::unload(m_sharedLibraryHandle);
        m_sharedLibraryHandle = nullptr;
    }
}

/* !!!!!!!!!!!!!!!!!!!!!!!!!! DefaultSharedLibrary !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!*/

ISlangUnknown* DefaultSharedLibrary::getInterface(const Guid& guid)
{
    return (guid == SlangSharedLibrary::IID_ISlangUnknown || guid == SlangSharedLibrary::IID_ISlangSharedLibrary) ? static_cast<ISlangSharedLibrary*>(this) : nullptr;
}

DefaultSharedLibrary::~DefaultSharedLibrary()
{
    if (m_sharedLibraryHandle)
    {
        SharedLibrary::unload(m_sharedLibraryHandle);
    }
}

SlangFuncPtr DefaultSharedLibrary::findFuncByName(char const* name)
{
    return SharedLibrary::findFuncByName(m_sharedLibraryHandle, name); 
}

} 
