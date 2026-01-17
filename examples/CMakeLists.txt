# Copyright (c) 2016-2025, Sascha Willems
# SPDX-License-Identifier: MIT

# 定义一个函数，function：函数关键词，buildExample：函数名，EXAMPLE_NAME：参数名
# Function for building single example
function(buildExample EXAMPLE_NAME)
	SET(EXAMPLE_FOLDER ${CMAKE_CURRENT_SOURCE_DIR}/${EXAMPLE_NAME})
	#message(STATUS "Generating project file for example in ${EXAMPLE_FOLDER}")
	# Main
	# 指定了多个目录下的多种文件
	file(GLOB SOURCE *.cpp ${BASE_HEADERS} ${EXAMPLE_FOLDER}/*.cpp)
	# 指定入口，默认使用 ${EXAMPLE_NAME}.cpp，若目录下存在 main.cpp，则使用 main.cpp
	SET(MAIN_CPP ${EXAMPLE_FOLDER}/${EXAMPLE_NAME}.cpp)
	if(EXISTS ${EXAMPLE_FOLDER}/main.cpp)
		SET(MAIN_CPP ${EXAMPLE_FOLDER}/main.cpp)
	ENDIF()
	if(EXISTS ${EXAMPLE_FOLDER}/${EXAMPLE_NAME}.h)
		SET(MAIN_HEADER ${EXAMPLE_FOLDER}/${EXAMPLE_NAME}.h)
	ENDIF()
	# imgui example requires additional source files
	IF(${EXAMPLE_NAME} STREQUAL "imgui")
		file(GLOB ADD_SOURCE "../external/imgui/*.cpp")
		# 追加资源
		SET(SOURCE ${SOURCE} ${ADD_SOURCE})
	ENDIF()
	# wayland requires additional source files
	IF(USE_WAYLAND_WSI)
		SET(SOURCE ${SOURCE} ${CMAKE_BINARY_DIR}/xdg-shell-client-protocol.h ${CMAKE_BINARY_DIR}/xdg-shell-protocol.c)
	ENDIF()
	# Add shaders
	# 处理着色器资源
	set(SHADER_DIR_GLSL "../shaders/glsl/${EXAMPLE_NAME}")
	file(GLOB SHADERS_GLSL "${SHADER_DIR_GLSL}/*.vert" "${SHADER_DIR_GLSL}/*.frag" "${SHADER_DIR_GLSL}/*.comp" "${SHADER_DIR_GLSL}/*.geom" "${SHADER_DIR_GLSL}/*.tesc" "${SHADER_DIR_GLSL}/*.tese" "${SHADER_DIR_GLSL}/*.mesh" "${SHADER_DIR_GLSL}/*.task" "${SHADER_DIR_GLSL}/*.rgen" "${SHADER_DIR_GLSL}/*.rchit" "${SHADER_DIR_GLSL}/*.rmiss" "${SHADER_DIR_GLSL}/*.rcall" "${SHADER_DIR_GLSL}/*.rahit" "${SHADER_DIR_GLSL}/*.rint" "${SHADER_DIR_GLSL}/*.glsl")
	set(SHADER_DIR_HLSL "../shaders/hlsl/${EXAMPLE_NAME}")
	file(GLOB SHADERS_HLSL "${SHADER_DIR_HLSL}/*.vert" "${SHADER_DIR_HLSL}/*.frag" "${SHADER_DIR_HLSL}/*.comp" "${SHADER_DIR_HLSL}/*.geom" "${SHADER_DIR_HLSL}/*.tesc" "${SHADER_DIR_HLSL}/*.tese" "${SHADER_DIR_HLSL}/*.mesh" "${SHADER_DIR_HLSL}/*.task" "${SHADER_DIR_HLSL}/*.rgen" "${SHADER_DIR_HLSL}/*.rchit" "${SHADER_DIR_HLSL}/*.rmiss" "${SHADER_DIR_HLSL}/*.rcall" "${SHADER_DIR_HLSL}/*.rahit" "${SHADER_DIR_HLSL}/*.rint")
	set(SHADER_DIR_SLANG "../shaders/slang/${EXAMPLE_NAME}")
	file(GLOB SHADERS_SLANG "${SHADER_DIR_SLANG}/*.slang")
	# IDE 分组，仅在部分软件中有效
	source_group("Shaders\\GLSL" FILES ${SHADERS_GLSL})
	source_group("Shaders\\HLSL" FILES ${SHADERS_HLSL})
	source_group("Shaders\\SLANG" FILES ${SHADERS_SLANG})
	# Add optional readme / tutorial
	# 添加 readme，同样是对 IDE 的处理
	file(GLOB README_FILES "${EXAMPLE_FOLDER}/*.md")
	if(WIN32)
		# 源文件列表
		add_executable(${EXAMPLE_NAME} WIN32 ${MAIN_CPP} ${SOURCE} ${MAIN_HEADER} ${SHADERS_GLSL} ${SHADERS_HLSL} ${SHADERS_SLANG} ${README_FILES})
		# 可执行文件库，外部链接库
		target_link_libraries(${EXAMPLE_NAME} base ${Vulkan_LIBRARY} ${WINLIBS})
	else(WIN32)
		add_executable(${EXAMPLE_NAME} ${MAIN_CPP} ${SOURCE} ${MAIN_HEADER} ${SHADERS_GLSL} ${SHADERS_HLSL} ${SHADERS_SLANG} ${README_FILES})
		target_link_libraries(${EXAMPLE_NAME} base )
	endif(WIN32)

	file(MAKE_DIRECTORY ${CMAKE_SOURCE_DIR}/bin)
	# 设置属性
	# PROPERTIES：关键词，后面跟键值对列表
	# VS_DEBUGGER_WORKING_DIRECTORY：开始调试时，目标进程所在目录位置
	set_target_properties(${EXAMPLE_NAME} PROPERTIES VS_DEBUGGER_WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}/bin)
	# 针对 texture3d 特殊库处理 
	if(${EXAMPLE_NAME} STREQUAL "texture3d")
		if(APPLE)
			# SRS - Use MacPorts paths as default since the same on x86 and Apple Silicon, can override for homebrew on cmake command line
			# 手动寻找资源
			if(NOT OpenMP_omp_LIBRARY AND EXISTS /opt/local/lib/libomp/libomp.dylib)
				set(OpenMP_omp_LIBRARY /opt/local/lib/libomp/libomp.dylib)
			endif()
			# 设置编译指令
			if(CMAKE_C_COMPILER_ID MATCHES "Clang\$")
				set(OpenMP_C_FLAGS "-Xclang -fopenmp")
				set(OpenMP_C_LIB_NAMES "omp")
				if(NOT OpenMP_C_INCLUDE_DIR AND EXISTS /opt/local/include/libomp)
					set(OpenMP_C_INCLUDE_DIR /opt/local/include/libomp)
				endif()
			endif()
			if(CMAKE_CXX_COMPILER_ID MATCHES "Clang\$")
				set(OpenMP_CXX_FLAGS "-Xclang -fopenmp")
				set(OpenMP_CXX_LIB_NAMES "omp")
				if(NOT OpenMP_CXX_INCLUDE_DIR AND EXISTS /opt/local/include/libomp)
					set(OpenMP_CXX_INCLUDE_DIR /opt/local/include/libomp)
				endif()
			endif()
		endif()
		find_package(OpenMP)
		if(OpenMP_CXX_FOUND)
			# 设置属性和资源
			set_target_properties(${EXAMPLE_NAME} PROPERTIES COMPILE_FLAGS ${OpenMP_CXX_FLAGS})
			# 指定头文件搜索路径
			# PUBLIC：当前项目使用，其他项目链接当前项目时也会使用
			# PRIVATE：当前项目使用，~ 不会使用
			# INTERFACE：当前项目不使用，~ 使用，常见于“纯头文件”库
			target_include_directories(${EXAMPLE_NAME} PRIVATE ${OpenMP_CXX_INCLUDE_DIRS})
			target_link_libraries(${EXAMPLE_NAME} ${OpenMP_CXX_LIBRARIES})
		endif()
	endif()

	# 尝试做安装板才会进这个分支
	if(RESOURCE_INSTALL_DIR)
		# TARGETS ${EXAMPLE_NAME}：目标为生成的可执行文件
		# DESTINATION ${CMAKE_INSTALL_BINDIR}：指定目标目录
		install(TARGETS ${EXAMPLE_NAME} DESTINATION ${CMAKE_INSTALL_BINDIR})
	endif()
endfunction(buildExample)

# 此处访问的 EXAMPLE 在后面才定义
# Cmake会在执行前扫描整个文件，并建立符号表，然后再运行，所以只要在文件结束之前有定义，就可以正常访问
# Build all examples
function(buildExamples)
	foreach(EXAMPLE ${EXAMPLES})
		buildExample(${EXAMPLE})
	endforeach(EXAMPLE)
endfunction(buildExamples)

# 定义了一个列表，内部存放名称
set(EXAMPLES
	bloom
	bufferdeviceaddress
	computecloth
	computecullandlod
	computeheadless
	computenbody
	computeparticles
	computeraytracing
	computeshader
	conditionalrender
	conservativeraster
	debugprintf
	debugutils
	deferred
	deferredmultisampling
	deferredshadows
	descriptorbuffer
	descriptorindexing
	descriptorsets
	displacement
	distancefieldfonts
	dynamicrendering
	dynamicrenderingmultisampling
	dynamicstate
	dynamicuniformbuffer	
	fragmentshaderbarycentrics
	gears
	geometryshader
	gltfloading
	gltfscenerendering
	gltfskinning
	graphicspipelinelibrary
	hdr
	hostimagecopy
	imgui
	indirectdraw
	inlineuniformblocks
	inputattachments
	instancing
	meshshader
	multisampling
	multithreading
	multiview
	negativeviewportheight	
	occlusionquery
	offscreen
	oit
	parallaxmapping
	particlesystem
	pbrbasic
	pbribl
	pbrtexture
	pipelines
	pipelinestatistics
	pushconstants
	pushdescriptors
	radialblur
	rayquery
	raytracingbasic
	raytracingcallable
	raytracinggltf
	raytracingintersection
	raytracingreflections
	raytracingpositionfetch
	raytracingsbtdata
	raytracingshadows
	raytracingtextures
	renderheadless
	screenshot
	shaderobjects
	shadowmapping
	shadowmappingomni
	shadowmappingcascade
	specializationconstants
	sphericalenvmapping
	ssao
	stencilbuffer
	subpasses
	terraintessellation
	tessellation
	textoverlay
	texture
	texture3d
	texturearray
	texturecubemap
	texturecubemaparray
	texturemipmapgen
	texturesparseresidency
	timelinesemaphore
	triangle
	trianglevulkan13
	variablerateshading
	vertexattributes
	viewportarray
	vulkanscene
)

# 调用
buildExamples()
