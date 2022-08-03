// Copyright 2020-2022 The Defold Foundation
// Copyright 2014-2020 King
// Copyright 2009-2014 Ragnar Svensson, Christian Murray
// Licensed under the Defold License version 1.0 (the "License"); you may not use
// this file except in compliance with the License.
// 
// You may obtain a copy of the License, together with FAQs at
// https://www.defold.com/license
// 
// Unless required by applicable law or agreed to in writing, software distributed
// under the License is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR
// CONDITIONS OF ANY KIND, either express or implied. See the License for the
// specific language governing permissions and limitations under the License.

package com.dynamo.bob.pipeline;

import static org.junit.Assert.assertEquals;
import static org.junit.Assert.assertTrue;

import java.io.ByteArrayOutputStream;
import java.io.IOException;
import java.io.InputStream;
import java.util.List;

import javax.vecmath.Quat4d;
import javax.vecmath.Vector3d;
import javax.vecmath.Vector4d;

import org.apache.commons.io.IOUtils;
import org.junit.Test;

import com.dynamo.bob.pipeline.LuaScanner.Property;
import com.dynamo.bob.pipeline.LuaScanner.Property.Status;

public class LuaScannerTest {

    String getFile(String file) throws IOException {
        InputStream input = getClass().getResourceAsStream(file);
        ByteArrayOutputStream output = new ByteArrayOutputStream(1024);
        IOUtils.copy(input, output);
        return new String(output.toByteArray());
    }

    private void assertValidRequire(String test, String expected) {
        LuaScanner scanner = new LuaScanner();
        scanner.parse(test);
        List<String> modules = scanner.getModules();
        assertTrue(modules.size() == 1);
        assertEquals(expected, modules.get(0));
    }

    @Test
    public void testScanner() throws Exception {
        String file = getFile("test_scanner.lua");
        LuaScanner scanner = new LuaScanner();
        scanner.parse(file);
        List<String> modules = scanner.getModules();

        assertEquals("a", modules.get(0));
        assertEquals("b", modules.get(1));
        assertEquals("c.d", modules.get(2));
        assertEquals("e.f", modules.get(3));
        assertEquals("g", modules.get(4));
        assertEquals("h", modules.get(5));
        assertEquals("i.j", modules.get(6));
        assertEquals("k.l", modules.get(7));
        assertEquals("m.n.o", modules.get(8));
        assertEquals("p.q.r", modules.get(9));
        assertEquals("s", modules.get(10));
        assertEquals("t", modules.get(11));
        assertEquals("foo1", modules.get(12));
        assertEquals("foo2", modules.get(13));
        assertEquals("foo3", modules.get(14));
        assertEquals("foo4", modules.get(15));
        assertEquals("foo5", modules.get(16));
        assertEquals("foo6", modules.get(17));
        assertEquals("foo7", modules.get(18));
        assertEquals("foo8", modules.get(19));
        assertEquals("foo9", modules.get(20));
        assertEquals("foo10", modules.get(21));
        assertEquals("foo11", modules.get(22));
        assertEquals("foo12", modules.get(23));
        assertEquals("foo13", modules.get(24));
        assertEquals("foo14", modules.get(25));
        assertEquals("foo15", modules.get(26));
        assertEquals("foo16", modules.get(27));
        assertTrue(modules.size() == 28);

        // test require detection with a trailing comment
        assertValidRequire("require \"foo.bar\" -- some comment", "foo.bar");
        assertValidRequire("require 'foo.bar' -- some comment", "foo.bar");
        assertValidRequire("require (\"foo.bar\") -- some comment", "foo.bar");
        assertValidRequire("require ('foo.bar') -- some comment", "foo.bar");

        // test require detection with a trailing empty comment
        assertValidRequire("require \"foo.bar\" --", "foo.bar");
        assertValidRequire("require 'foo.bar' --", "foo.bar");
        assertValidRequire("require (\"foo.bar\") --", "foo.bar");
        assertValidRequire("require ('foo.bar') --", "foo.bar");

        // test require detection with a trailing multi-line comment
        assertValidRequire("require \"foo.bar\" --[[ some comment]]--", "foo.bar");
        assertValidRequire("require 'foo.bar' --[[ some comment]]--", "foo.bar");
        assertValidRequire("require (\"foo.bar\") --[[ some comment]]--", "foo.bar");
        assertValidRequire("require ('foo.bar') --[[ some comment]]--", "foo.bar");
    }

    private Property findProperty(List<Property> properties, String name) {
        for (Property p : properties) {
            if (p.name != null && p.name.equals(name)) {
                return p;
            }
        }
        return null;
    }

    private void assertProperty(List<Property> properties, String name, Object value, int line) {
        Property property = findProperty(properties, name);
        if (property != null && property.status == Status.OK && property.name.equals(name)) {
            assertEquals(value, property.value);
            assertEquals(line, property.line);
            return;
        }
        assertTrue(false);
    }

    private void assertPropertyStatus(List<Property> properties, String name, Status status, int line) {
        Property property = findProperty(properties, name);
        assertTrue(property != null && property.status == status && property.line == line);
    }

    private List<Property> getPropertiesFromString(String s) throws IOException {
        LuaScanner scanner = new LuaScanner();
        scanner.parse(s);
        return scanner.getProperties();
    }

    private List<Property> getPropertiesFromFile(String path) throws IOException {
        return getPropertiesFromString(getFile(path));
    }

    @Test
    public void testProps() throws Exception {
        List<Property> properties = getPropertiesFromFile("test_props.lua");

        assertEquals(8, properties.size());
        assertProperty(properties, "prop1", new Double(0), 10);
        assertProperty(properties, "prop2", new Double(0), 13);
        assertProperty(properties, "prop3", new Double(0), 14);
        assertProperty(properties, "prop4", new Double(0), 15);
        assertProperty(properties, "prop5", new Double(0), 16);
        assertEquals(Status.INVALID_ARGS, properties.get(5).status);
        assertPropertyStatus(properties, "three_args", Status.INVALID_VALUE, 19);
        assertPropertyStatus(properties, "unknown_type", Status.INVALID_VALUE, 20);
    }

    @Test
    public void testPropsStripped() throws Exception {
        String source = getFile("test_props.lua");
        LuaScanner scanner = new LuaScanner();
        source = scanner.parse(source);
        List<Property> properties = scanner.getProperties();
        assertEquals(8, properties.size());
        assertProperty(properties, "prop1", new Double(0), 10);
        assertProperty(properties, "prop2", new Double(0), 13);
        assertProperty(properties, "prop3", new Double(0), 14);
        assertProperty(properties, "prop4", new Double(0), 15);
        assertProperty(properties, "prop5", new Double(0), 16);
        assertEquals(Status.INVALID_ARGS, properties.get(5).status);
        assertPropertyStatus(properties, "three_args", Status.INVALID_VALUE, 19);
        assertPropertyStatus(properties, "unknown_type", Status.INVALID_VALUE, 20);
        
        // parse the already stripped source
        // there should be no properties left
        scanner.parse(source);
        properties = scanner.getProperties();
        assertEquals(0, properties.size());
    }

    @Test
    public void testPropsNumber() throws Exception {
        List<Property> properties = getPropertiesFromFile("test_props_number.lua");

        assertEquals(4, properties.size());
        assertProperty(properties, "prop1", new Double(12), 0);
        assertProperty(properties, "prop2", new Double(1.0), 1);
        assertProperty(properties, "prop3", new Double(.1), 2);
        assertProperty(properties, "prop4", new Double(-1.0E2), 3);
    }

    @Test
    public void testPropsHash() throws Exception {
        List<Property> properties = getPropertiesFromFile("test_props_hash.lua");

        assertEquals(4, properties.size());
        assertProperty(properties, "prop1", "hash", 0);
        assertProperty(properties, "prop2", "", 1);
        assertPropertyStatus(properties, "prop3", Status.INVALID_VALUE, 2);
        assertProperty(properties, "prop4", "hash", 3);
    }

    @Test
    public void testPropsUrl() throws Exception {
        List<Property> properties = getPropertiesFromFile("test_props_url.lua");

        assertEquals(4, properties.size());
        assertProperty(properties, "prop1", "url", 0);
        assertProperty(properties, "prop2", "", 1);
        assertProperty(properties, "prop3", "", 2);
        assertProperty(properties, "prop4", "url", 3);
    }

    @Test
    public void testPropsVec3() throws Exception {
        List<Property> properties = getPropertiesFromFile("test_props_vec3.lua");

        assertEquals(3, properties.size());
        assertProperty(properties, "prop1", new Vector3d(), 0);
        assertProperty(properties, "prop2", new Vector3d(1, 2, 3), 1);
        assertProperty(properties, "prop4", new Vector3d(2, 2, 2), 3);
    }

    @Test
    public void testPropsVec4() throws Exception {
        List<Property> properties = getPropertiesFromFile("test_props_vec4.lua");

        assertEquals(3, properties.size());
        assertProperty(properties, "prop1", new Vector4d(), 0);
        assertProperty(properties, "prop2", new Vector4d(1, 2, 3, 4), 1);
        assertProperty(properties, "prop4", new Vector4d(2, 2, 2, 2), 3);
    }

    @Test
    public void testPropsQuat() throws Exception {
        List<Property> properties = getPropertiesFromFile("test_props_quat.lua");

        assertEquals(2, properties.size());
        assertProperty(properties, "prop1", new Quat4d(), 0);
        Quat4d q = new Quat4d();
        q.set(1, 2, 3, 4);
        assertProperty(properties, "prop2", q, 1);
    }

    @Test
    public void testPropsBool() throws Exception {
        List<Property> properties = getPropertiesFromFile("test_props_bool.lua");

        assertEquals(2, properties.size());
        assertProperty(properties, "prop1", true, 0);
        assertProperty(properties, "prop2", false, 1);
    }

    @Test
    public void testPropsMaterial() throws Exception {
        List<Property> properties = getPropertiesFromFile("test_props_material.lua");

        assertEquals(4, properties.size());
        assertProperty(properties, "prop1", "material", 0);
        assertProperty(properties, "prop2", "", 1);
        assertProperty(properties, "prop3", "", 2);
        assertProperty(properties, "prop4", "material", 3);
    }

}
