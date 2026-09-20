#include "platform_paths.h"

#include <pwd.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <vector>
#include <array>

// #include <iomanip>
#ifdef PLATFORM_LINUX_M
#include <libgen.h>
#include <linux/limits.h>
#include <unistd.h>

#endif

#ifdef PLATFORM_APPLE_M
#include <CoreFoundation/CoreFoundation.h>
#include <OpenDirectory/OpenDirectory.h>
// #include <SystemAdministration/SystemAdministration.h>
#include <mach-o/dyld.h>

#endif

#ifdef PLATFORM_LINUX_M
std::string getExecutablePath()
{
    std::array<char, PATH_MAX> result;
    ssize_t count = readlink("/proc/self/exe", result.data(), PATH_MAX);
    const char* path;
    if (count != -1)
    {
        path = dirname(result.data());
    }
    return std::string(path);
}

#endif

#ifdef PLATFORM_APPLE_M

std::string getExecutablePath()
{
    std::array<char, 2048> path;
    uint32_t size = path.size();

    if (_NSGetExecutablePath(path.data(), &size) != 0)
    {
        printf("Buffer too small; need size %u\n", size);
    }

    return std::string(path.data());
}

#endif

// TODO: Move to a more general location
/*static std::pair<std::string, bool> getUsername()
{
    char username_buffer[256];

    if(getlogin_r(username_buffer, 256) == 0)
    {
        return std::make_pair<>(std::string{username_buffer}, true);
    }
    else
    {
        return std::make_pair<>("", false);
    }
}*/

static std::pair<std::string, bool> getHomeDirPath()
{
    const char* home_dir = getenv("HOME");

    if (!home_dir)
    {
        const struct passwd* const pwd = getpwuid(getuid());
        if (pwd)
        {
            home_dir = pwd->pw_dir;
        }
        else
        {
            return std::make_pair<>("", false);
        }
    }

    return std::make_pair<>(home_dir, true);
}

/*
#ifdef PLATFORM_LINUX_M
    const std::pair<std::string, bool> username_and_status{getUsername()};

    if (!username_and_status.second)
    {
        LUMOS_LOG_ERROR() << "Failed to get login name! Exiting...";
        is_valid_ = false;
        return;
    }
    const lumos::filesystem::path home_dir_path("/home/" + username_and_status.first);

    const lumos::filesystem::path config_dir_path{home_dir_path / lumos::filesystem::path{configuration_folder_name}};

    if (!lumos::filesystem::exists(config_dir_path))
    {
        LUMOS_LOG_INFO() << config_dir_path << " dir does not exist, creating...";
        try
        {
            lumos::filesystem::create_directory(config_dir_path);
        }
        catch (const std::exception& e)
        {
            LUMOS_LOG_ERROR() << "Failed to create config directory at \"" << config_dir_path
                            << "\". Exception: " << e.what();
            is_valid_ = false;
            return;
        }
    }

    const lumos::filesystem::path duoplot_dir_path{config_dir_path / lumos::filesystem::path{"duoplot"}};
#endif

#ifdef PLATFORM_APPLE_M

#endif

#ifdef PLATFORM_WINDOWS_M
    const lumos::filesystem::path config_dir_path{home_dir_path / lumos::filesystem::path{configuration_folder_name}};
#endif

*/

// #ifdef PLATFORM_APPLE_M

std::string getUsername()
{
    const char* username = getenv("USER");

    if (username)
    {
        return username;
    }
    else
    {
        return "";
    }

    /*std::string username_str = "";

    CFStringRef usernameRef = ODRecordCopyValues(ODRecordCopyCurrentUser(NULL), kODAttributeTypeRecordName, NULL);

    // Check if the username reference is valid
    if (usernameRef)
    {
        // Convert CFStringRef to C string
        char username[PATH_MAX];
        if (CFStringGetCString(usernameRef, username, PATH_MAX, kCFStringEncodingUTF8))
        {
            // Print the username
            username_str = username;
        }

        // Release the CFStringRef
        CFRelease(usernameRef);
    }

    std::cout << "Username: " << username_str << std::endl;

    return username_str;*/
}

#ifdef PLATFORM_APPLE_M

std::string getConfigDirRoot()
{
    return "/Users/" + getUsername() + "/Library/Preferences";
}

std::string getApplicationRootPath()
{
    CFBundleRef mainBundle = CFBundleGetMainBundle();

    std::string res = "/Applications/duoplot.app/";

    if (mainBundle)
    {
        CFURLRef bundleURL = CFBundleCopyBundleURL(mainBundle);

        if (bundleURL)
        {
            char bundlePath[PATH_MAX];
            if (CFURLGetFileSystemRepresentation(bundleURL, true, (UInt8*)bundlePath, PATH_MAX))
            {
                res = bundlePath;
            }

            CFRelease(bundleURL);
        }
    }

    return res;
}

#endif

#ifdef PLATFORM_LINUX_M

std::string getConfigDirRoot()
{
    const char* xdg_config = getenv("XDG_CONFIG_HOME");
    if (xdg_config)
    {
        return std::string(xdg_config);
    }
    return getHomeDirPath().first + "/.config";
}

#endif

lumos::filesystem::path getConfigDir()
{
    // The path received from getConfigDirRoot() is assumed to exist
    return lumos::filesystem::path{getConfigDirRoot() / lumos::filesystem::path{"duoplot"}};
}

std::string getResourcesPathString()
{
    // return getApplicationRootPath() + "/Contents/Resources/";
    return "../resources/";
}
