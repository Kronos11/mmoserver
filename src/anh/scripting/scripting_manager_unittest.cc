/*
 This file is part of MMOServer. For more information, visit http://swganh.com
 
 Copyright (c) 2006 - 2010 The SWG:ANH Team

 MMOServer is free software: you can redistribute it and/or modify
 it under the terms of the GNU General Public License as published by
 the Free Software Foundation, either version 3 of the License, or
 (at your option) any later version.

 MMOServer is distributed in the hope that it will be useful,
 but WITHOUT ANY WARRANTY; without even the implied warranty of
 MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 GNU General Public License for more details.

 You should have received a copy of the GNU General Public License
 along with MMOServer.  If not, see <http://www.gnu.org/licenses/>.
*/
#include "scripting_manager.h"
#include <boost/python.hpp>
#include <anh/scripting/scripting_modules_unittest.h>
#include <gtest/gtest.h>
#include <iostream>
#include <cstdint>

using namespace anh::scripting;
using namespace anh::component;
using namespace boost::python;

// this is used for embedding, so we can have our bindings in another file
void baseDerive();
void componentDerive();
BOOST_PYTHON_MODULE(embedded_hello)
{
    baseDerive();
}
BOOST_PYTHON_MODULE(embedded_component)
{
    anh::component::componentDerive();
}

namespace {
class ScriptEngineTest : public ::testing::Test 
{
 protected:
     virtual void SetUp() 
     {
         e = std::make_shared<ScriptingManager>("../../bin/debug/scripts/unittests/");
     }
     std::shared_ptr<ScriptingManager> e;
};
TEST_F(ScriptEngineTest, loadScript ) 
{
    e->load("test.py");
    EXPECT_TRUE(e->isFileLoaded("test.py"));
}

TEST_F(ScriptEngineTest, runLoadedScript) 
{
    e->load("test.py");
    EXPECT_TRUE(e->isFileLoaded("test.py"));
    EXPECT_NO_THROW(e->run("test.py"));
}
TEST_F(ScriptEngineTest, runNonLoadedScript) 
{
    EXPECT_NO_THROW(e->run("nonloadedscript.py"));
    std::string err_msg ("No such file or directory");
    EXPECT_TRUE(e->getErrorMessage().find(err_msg) != std::string::npos);
}
TEST_F(ScriptEngineTest, runSecondLoadedScript)
{
    e->load("test.py");
    e->load("testRunSecond.py");
    EXPECT_TRUE(e->isFileLoaded("testRunSecond.py"));
    EXPECT_NO_THROW(e->run("testRunSecond.py"));
}
TEST_F(ScriptEngineTest, cantLoadScript)
{
    e->load("noscript.py");
    EXPECT_FALSE(e->isFileLoaded("noscript.py"));
    std::string err_msg ("No such file or directory");
    EXPECT_TRUE(e->getErrorMessage().find(err_msg) != std::string::npos);
}
TEST_F(ScriptEngineTest, loadSameFileTwice)
{
    e->load("test.py");
    e->load("test.py");
    e->run("test.py");
    EXPECT_TRUE(e->isFileLoaded("test.py"));
}
TEST_F(ScriptEngineTest, loadFileOneDeep)
{
    e->load("test-deep/scripta.py");
    EXPECT_TRUE(e->isFileLoaded("test-deep/scripta.py"));
}
TEST_F(ScriptEngineTest, getLoadedFiles)
{
    e->load("../scripta.py");
    e->load("../scriptb.py");
    e->load("test.py");
    e->load("testRunSecond.py");
    e->load("test-deep/scripta.py");
    EXPECT_EQ(uint32_t(5),e->getLoadedFiles().size());
}
TEST_F(ScriptEngineTest, removeLoadedFile)
{
    e->load("test.py");
    EXPECT_TRUE(e->isFileLoaded("test.py"));
    e->removeFile("test.py");
    EXPECT_FALSE(e->isFileLoaded("test.py"));
}
TEST_F(ScriptEngineTest, reloadFile)
{
    e->load("test.py");
    EXPECT_TRUE(e->isFileLoaded("test.py"));
    e->reload("test.py");
    EXPECT_TRUE(e->isFileLoaded("test.py"));
}
TEST_F(ScriptEngineTest, getPythonException)
{
    e->load("noscript.py");
    std::string err_msg ("No such file or directory");
    EXPECT_TRUE(e->getErrorMessage().find(err_msg) != std::string::npos);
}

TEST_F(ScriptEngineTest, getValueFromPython)
{
    _inittab module;
    module.name = "embedded_hello";
    module.initfunc = PyInit_embedded_hello;
    CppDerived cpp;
    EXPECT_EQ("Hello from C++!", cpp.hello());
    object obj (e->embed("embedded_hello.py", "PythonDerived", module));
    object py_base = obj();
    Base& py = extract<Base&>(py_base) BOOST_EXTRACT_WORKAROUND;
    EXPECT_EQ("Hello from Python!", py.hello());
}

TEST_F(ScriptEngineTest, getComponentFromPython)
{
    _inittab module;
    module.name = "embedded_component";
    module.initfunc = PyInit_embedded_component;

    object obj (e->embed("embedded_component.py", "DerivedComponent", module));
    object py_base = obj();
    try {
        ComponentInterface& comp = extract<ComponentInterface&>(py_base);
        ObjectId id = comp.object_id();
        EXPECT_EQ(0xDEADBEEF, id);
    }
    catch(...)
    {
        e->getExceptionFromPy_();
        std::string err = e->getErrorMessage();
    }
}

TEST_F(ScriptEngineTest, getRadialComponentFromPython)
{
    _inittab module;
    module.name = "embedded_component";
    module.initfunc = PyInit_embedded_component;

    object obj (e->embed("radial_component.py", "RadialComponent", module));
    object py_base = obj();
    try {
        ComponentInterface& comp = extract<ComponentInterface&>(py_base);

        ObjectId id = comp.object_id();
        EXPECT_EQ(0xDEADBEEF, id);
    }
    catch(...)
    {
        e->getExceptionFromPy_();
        std::string err = e->getErrorMessage();
    }
}

TEST_F(ScriptEngineTest, getHAMComponentFromPython)
{
    _inittab module;
    module.name = "embedded_component";
    module.initfunc = PyInit_embedded_component;

    object obj (e->embed("ham_component.py", "HamComponent", module));
    object py_base = obj();
    try {
        HAMComponentInterface& comp = extract<HAMComponentInterface&>(py_base);
        boost::property_tree::ptree pt;
        
        comp.Init(pt);
        comp.Update(15.0);
        
        ObjectId id = comp.object_id();
        EXPECT_EQ(0xDEADBEEF, id);
    }
    catch(...)
    {
        e->getExceptionFromPy_();
        std::string err = e->getErrorMessage();
    }
}

} // anon namespace
