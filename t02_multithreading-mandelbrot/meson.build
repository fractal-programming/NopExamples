
project(
	'Multithreaded Mandelbrot',
	'cpp',
)

fs = import('fs')
cxx = meson.get_compiler('cpp')

# Sources

srcs = [
	'../deps/SystemCore/Processing.cpp',
	'../deps/SystemCore/Log.cpp',
	'../deps/SystemCore/SystemDebugging.cpp',
	'../deps/SystemCore/SystemCommanding.cpp',
	'../deps/SystemCore/TcpListening.cpp',
	'../deps/SystemCore/TcpTransfering.cpp',
	'../deps/LibNaegCommon/ThreadPooling.cpp',
	'../deps/LibNaegCommon/TelnetFiltering.cpp',
	'../deps/LibNaegCommon/KeyUser.cpp',
	'../deps/LibNaegCommon/LibTime.cpp',
	'../deps/LibNaegCommon/LibFilesys.cpp',
	'../deps/LibNaegCommon/LibDspc.cpp',
	'../deps/LibNaegCommon/Res.cpp',
	'main.cpp',
	'Supervising.cpp',
	'MandelbrotCreating.cpp',
	'MandelBlockFilling.cpp',
	'UserInteracting.cpp',
	'LibBmp.cpp',
	'LibMandel.cpp',
]

# Arguments

args = [
	'-DdPackageName="' + meson.project_name() + '"',
	'-DdAppName="app"',
	'-DdVersion="app-v1.26.3.1"',
	'-DCONFIG_PROC_HAVE_LOG=1',
	'-DCONFIG_PROC_HAVE_CORE_LOG=0',
]

# https://gcc.gnu.org/onlinedocs/gcc/Warning-Options.html
warningsGcc = [
	'-Wall',
	'-Wextra',
	'-Wpedantic',
	'-Werror',
	'-Wfatal-errors',
	'-Wreorder',
	'-Wswitch-enum',
	'-Wuseless-cast',
	'-Wparentheses',
	'-Wshift-overflow',
	'-Wsign-compare',
	'-Wzero-as-null-pointer-constant',
	'-Wcast-align',
	'-Wcast-qual',
	'-Wcatch-value',
	'-Wchar-subscripts',
	'-Wswitch-default',
	'-Wctor-dtor-privacy',
	'-Wduplicated-branches',
	'-Wduplicated-cond',
	'-Wempty-body',
	'-Wextra-semi',
	'-Wfloat-equal',
	'-Wformat',
	'-Wformat-extra-args',
	'-Wimplicit-fallthrough',
	'-Wmissing-field-initializers',
	'-Wnull-dereference',
	'-Wshadow',
]

warningsClang = [
	'-Wall',
	'-Wextra',
	'-Wpedantic',
	'-Werror',
	'-Wno-gnu-zero-variadic-macro-arguments',
	'-Wshadow',
	'-Wsign-conversion',
	'-Wnull-dereference',
	'-Wdouble-promotion',
	'-Wimplicit-fallthrough',
	'-Wcast-qual',
	'-Wcast-align',
	'-Wnon-virtual-dtor',
	'-Woverloaded-virtual',
	'-Wfloat-equal',
	'-Wswitch-enum',
	'-Wmissing-declarations',
	'-Wunreachable-code',
	'-Wdocumentation',
	'-Wthread-safety',
]

if host_machine.system() == 'windows'
	args += '-D_WIN32_WINNT=_WIN32_WINNT_WIN10'
	args += '-DWINVER=_WIN32_WINNT_WIN10'

	# wrestool -l CodeOrb.exe
	if cxx.get_id() != 'msvc'
		args += '-D__STDCPP_THREADS__=1'
	endif
else
	args += '-std=gnu++11'
	args += '-O3'
	#args += '-ffast-math'
	args += '-march=native'
	args += '-ffp-contract=off'
endif

if cxx.get_id() == 'msvc'
	args += '/std:c++17'
elif cxx.get_id() == 'gcc'
	args += warningsGcc
elif cxx.get_id() == 'clang'
	args += warningsClang
endif

# Dependencies

deps = []
deps += dependency('threads')

if host_machine.system() == 'windows'
	deps += cxx.find_library('ws2_32')
endif

if fs.is_dir('../deps/tclap_loc')
	message('Submodule tclap found')

	incTclap = include_directories(['../deps/tclap_loc/include'], is_system: true)
	deps += declare_dependency(include_directories: incTclap)
	args += '-DAPP_HAS_TCLAP=1'
else
	warning('Submodule tclap not found. Limited CLI options')
endif

if cxx.get_id() == 'gcc'
	have_avx2 = cxx.has_argument('-mavx2')
	if have_avx2
		add_project_arguments('-mavx2', language : 'cpp')
		args += '-DAPP_HAS_AVX2=1'
	else
		warning('Scalar code only')
	endif
endif

dep_vulkan = dependency('vulkan', required : false)
if dep_vulkan.found()
	deps += dep_vulkan
	args += '-DAPP_HAS_VULKAN=1'

	srcs += 'vulkan/LibVulkan.cpp'
	srcs += 'vulkan/DeviceVulkan.cpp'
	srcs += 'vulkan/StructComputing.cpp'
	srcs += 'vulkan/VulkanComputing.cpp'
endif

# Application

nameExe = 'app'
if host_machine.system() == 'windows'
	nameExe = 'App'
endif

myApp = executable(
	nameExe,
	[
		srcs,
	],
	include_directories : include_directories([
		'../deps/SystemCore',
		'../deps/LibNaegCommon',
		'vulkan',
	]),
	dependencies : [
		deps,
	],
	cpp_args : [
		args,
	],
)

