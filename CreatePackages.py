import os
import subprocess
import sys
from py7zr import SevenZipFile

def find_vcvarsall(version=17.7):
    return r'C:\Program Files\Microsoft Visual Studio\2022\Professional\VC\Auxiliary\Build\vcvars64.bat'

def removeDuplicates(variable):
    """Remove duplicate values of an environment variable."""
    old_list = variable.split(os.pathsep)
    new_list = []
    for i in old_list:
        if i not in new_list:
            new_list.append(i)
    new_variable = os.pathsep.join(new_list)
    return new_variable

def query_vcvarsall(version, arch="x64"):
    """Launch vcvarsall.bat and read the settings from its environment"""
    vcvarsall = find_vcvarsall()
    interesting = set(("include", "lib", "libpath", "path"))
    result = {}

    if vcvarsall is None:
        raise Exception("Unable to find vcvarsall.bat")
    print("Calling 'vcvarsall.bat %s' (version=%s)" % (arch, version))
    popen = subprocess.Popen('"%s" %s & set' % (vcvarsall, arch),
                             stdout=subprocess.PIPE,
                             stderr=subprocess.PIPE)

    stdout, stderr = popen.communicate()
    if popen.wait() != 0:
        # raise PackagingPlatformError(stderr.decode("mbcs"))
        raise Exception(stderr.decode("mbcs"))

    stdout = stdout.decode("mbcs")
    for line in stdout.split("\n"):
        if '=' not in line:
            continue
        line = line.strip()
        key, value = line.split('=', 1)
        key = key.lower()
        if key in interesting:
            if value.endswith(os.pathsep):
                value = value[:-1]
            result[key] = removeDuplicates(value)

    if len(result) != len(interesting):
        raise ValueError(str(list(result)))

    return result


def build_solution(solution_file_path, build_config_platform, build_action):
    if not os.path.exists(solution_file_path):
        print('Solution file not found: %s' % solution_file_path)
        return -1

    build_action = build_action.lower()
    command = 'devenv %s /%s \"%s\"' % (solution_file_path, build_action, build_config_platform)
    print('Running Command : [%s]' % command)
    os.system(command)
    print('\n')


def read_version_info():
    version_header = r'source/version.h'
    major, minor, patch, build = 0, 0, 0, 0  # Default values

    try:
        with open(version_header, 'r') as version_file:
            for line in version_file:
                if line.startswith('#define AT_VERSION_MAJOR'):
                    major = int(line.split()[2])
                elif line.startswith('#define AT_VERSION_MINOR'):
                    minor = int(line.split()[2])
                elif line.startswith('#define AT_VERSION_PATCH'):
                    patch = int(line.split()[2])
                elif line.startswith('#define AT_VERSION_BUILD'):
                    build = int(line.split()[2])
    except FileNotFoundError:
        print(f"Warning: Version header file '{version_header}' not found. Using default version 'unknown'.")

    return f"{major}.{minor}.{patch}.{build}"


def create_7z_archive(output_filename, files_to_pack):
    with SevenZipFile(output_filename, 'w') as archive:
        for file_path in files_to_pack:
            if os.path.exists(file_path):
                archive.write(file_path, os.path.basename(file_path))
            else:
                print(f"Warning: File '{file_path}' not found. Skipping.")


def create_package(config_name):
    output_filename = r'Releases\AltTab_{}_{}_x64.7z'.format(read_version_info(), config_name)
    files_to_pack = [r'x64\%s\AltTab.exe' % config_name, 'AltTab.chm', 'ReadMe.txt', 'ReleaseNotes.txt']

    create_7z_archive(output_filename, files_to_pack)
    print(f"Package '{output_filename}' created successfully.")

if __name__ == "__main__":
    vc_env = query_vcvarsall(17.7)
    os.environ["Path"] = vc_env["path"]

    if True:
        build_solution(r'AltTab.sln', 'Release|x64', 'clean')
        build_solution(r'AltTab.sln', 'ReleaseNoLogger|x64', 'clean')

        build_solution(r'AltTab.sln', 'Release|x64', 'build')
        build_solution(r'AltTab.sln', 'ReleaseNoLogger|x64', 'build')

    create_package('Release')
    create_package('ReleaseNoLogger')
