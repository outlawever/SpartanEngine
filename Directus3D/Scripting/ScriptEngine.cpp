/*
Copyright(c) 2016-2017 Panos Karabelas

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and / or sell
copies of the Software, and to permit persons to whom the Software is furnished
to do so, subject to the following conditions :

The above copyright notice and this permission notice shall be included in
all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS
FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.IN NO EVENT SHALL THE AUTHORS OR
COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER
IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
*/

//= INCLUDES =========================
#include "ScriptEngine.h"
#include "../Core/Context.h"
#include "../Logging/Log.h"
#include <mono/jit/jit.h>
#include <mono/metadata/assembly.h>
#include <mono/metadata/loader.h>
#include <mono/metadata/metadata.h>
#include <mono/metadata/mono-config.h>
//====================================

//= NAMESPACES =====
using namespace std;
//==================

MonoDomain* domain;
const char* monoAssemblyPath = "Data\\Mono\\lib";
const char* monoConfigurationPath = "Data\\Mono\\etc";
const char* domainName = "Directus3DMono";
const char* runtimeVersion = "v4.0.30319";

void PrintHello()
{
	LOG_INFO("Hello");
}

ScriptEngine::ScriptEngine(Context* context) : Subsystem(context)
{
	// Tell Mono where to find it's assembly and configuration files
	mono_set_dirs(monoAssemblyPath, monoConfigurationPath);

	// Initialize the JIT runtime 
	domain = mono_jit_init_version(domainName, runtimeVersion);

	if (!domain)
		LOG_ERROR("Failed to initialize JIT runtime");

	//= TEST/EXPERIMENT ===========================================================================
	MonoAssembly* assembly = mono_domain_assembly_open(mono_domain_get(), "Directus3DRuntime.dll");
	if (!assembly)
		LOG_ERROR("Failed to load assembly.");

	MonoImage* image = mono_assembly_get_image(assembly);

	mono_add_internal_call("ClassName::FunctionName", PrintHello);

	MonoClass* mclass = mono_class_from_name(image, "", "");
	MonoMethod* method = mono_class_get_method_from_name(mclass, "Print", 0);
	//=============================================================================================
}

ScriptEngine::~ScriptEngine()
{
	mono_jit_cleanup(domain);
}

void ScriptEngine::Reset()
{

}