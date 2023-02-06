

LIBDIR := /home/gherron/projects/frameworks/libs
VPATH = $(LIBDIR)/imgui-master $(LIBDIR)/imgui-master/backends shaders

SDK = /home/gherron/vulkanSDK/1.3.204.1/x86_64/include

pkgName=rtrtFramework
pkgDir = /home/gherron/packages
extraFiles = Makefile rtrt.vcxproj rtrt.sln

CXX = g++

OPTIMIZE = -g

CXXFLAGS = -DPROJECT_NAME=\"rtrt\" -DPROJECT_RELDIRECTORY=\".\"  $(OPTIMIZE) -std=c++17 -I. -I$(SDK) -I$(LIBDIR)/glm -I$(LIBDIR)/imgui-master -I$(LIBDIR)/imgui-master/backends -I$(LIBDIR)  -I$(LIBDIR)/glfw/include

LIBS =  -lassimp  -lvulkan -ldl -lpthread -lX11 -lXxf86vm -lXrandr -lXi `pkg-config --static --libs glfw3`

target = rtrt.exe

shader_spvs = spv/post.frag.spv  spv/post.vert.spv
shader_src =  shaders/post.frag shaders/post.vert shaders/shared_structs.h 

headers = app.h vkapp.h camera.h buffer_wrap.h descriptor_wrap.h image_wrap.h extensions_vk.hpp
src = app.cpp vkapp.cpp camera.cpp vkapp_fns.cpp extensions_vk.cpp

imgui_src = 

objects = $(patsubst %.cpp,%.o,$(src)) $(patsubst %.cpp,%.o,$(imgui_src)) 

$(target): $(objects) $(shader_spvs)
	g++  $(CXXFLAGS) -o $@  $(objects) $(LIBS)

spv/denoiseX.comp.spv: shaders/denoiseX.comp shaders/shared_structs.h
	mkdir -p spv
	glslangValidator -g --target-env vulkan1.2 -o $@  $<
spv/denoiseY.comp.spv: shaders/denoiseY.comp shaders/shared_structs.h
	mkdir -p spv
	glslangValidator -g --target-env vulkan1.2 -o $@  $<
spv/post.frag.spv: shaders/post.frag shaders/shared_structs.h
	mkdir -p spv
	glslangValidator -g --target-env vulkan1.2 -o $@  $<
spv/post.vert.spv: shaders/post.vert shaders/shared_structs.h
	mkdir -p spv
	glslangValidator -g --target-env vulkan1.2 -o $@  $<
spv/raytrace.rchit.spv: shaders/raytrace.rchit shaders/shared_structs.h
	mkdir -p spv
	glslangValidator -g --target-env vulkan1.2 -o $@  $<
spv/raytrace.rgen.spv: shaders/raytrace.rgen shaders/shared_structs.h
	mkdir -p spv
	glslangValidator -g --target-env vulkan1.2 -o $@  $<
spv/raytrace.rmiss.spv: shaders/raytrace.rmiss shaders/shared_structs.h
	mkdir -p spv
	glslangValidator -g --target-env vulkan1.2 -o $@  $<
spv/raytraceShadow.rmiss.spv: shaders/raytraceShadow.rmiss shaders/shared_structs.h
	mkdir -p spv
	glslangValidator -g --target-env vulkan1.2 -o $@  $<
spv/scanline.frag.spv: shaders/scanline.frag shaders/shared_structs.h
	mkdir -p spv
	glslangValidator -g --target-env vulkan1.2 -o $@  $<
spv/scanline.vert.spv: shaders/scanline.vert shaders/shared_structs.h
	mkdir -p spv
	glslangValidator -g --target-env vulkan1.2 -o $@  $<

test:
	ls -1 spv

%.o: %.cpp
	$(CXX) $(CXXFLAGS) -c $< -o $@

run: $(target)  $(objects)
	./rtrt.exe

rund: $(target)  $(objects)
	./rtrt.exe -d

clean:
	rm -rf *.suo *.sdf *.orig Release Debug ipch *.o *~ raytrace dependencies *13*scn  *13*ppm

zip:
	rm -rf $(pkgDir)/$(pkgName) $(pkgDir)/$(pkgName).zip
	mkdir $(pkgDir)/$(pkgName)
	mkdir $(pkgDir)/$(pkgName)/src
	mkdir $(pkgDir)/$(pkgName)/src/shaders
	mkdir $(pkgDir)/$(pkgName)/src/spv
	mkdir $(pkgDir)/$(pkgName)/libs
	cp $(src) $(headers) $(pkgDir)/$(pkgName)/src
	cp $(shader_src) $(pkgDir)/$(pkgName)/src/shaders
	cp $(shader_spvs) $(pkgDir)/$(pkgName)/src/spv
	cp -r ../libs/* $(pkgDir)/$(pkgName)/libs
	cp -r $(extraFiles) $(pkgDir)/$(pkgName)/src
	cd $(pkgDir)/$(pkgName)/src;  unifdef -DSTUDENT -m  $(src)   $(srcFiles) || /bin/true

	cd $(pkgDir);  zip -r $(pkgName).zip $(pkgName); rm -rf $(pkgName)
	cd /home/gherron/projects/dev-rtrt; unzip $(pkgDir)/$(pkgName).zip

dependencies: 
	g++ -MM $(CXXFLAGS)  $(src) > dependencies

include dependencies
