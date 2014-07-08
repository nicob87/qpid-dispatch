##
## Licensed to the Apache Software Foundation (ASF) under one
## or more contributor license agreements.  See the NOTICE file
## distributed with this work for additional information
## regarding copyright ownership.  The ASF licenses this file
## to you under the Apache License, Version 2.0 (the
## "License"); you may not use this file except in compliance
## with the License.  You may obtain a copy of the License at
##
##   http://www.apache.org/licenses/LICENSE-2.0
##
## Unless required by applicable law or agreed to in writing,
## software distributed under the License is distributed on an
## "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY
## KIND, either express or implied.  See the License for the
## specific language governing permissions and limitations
## under the License
##

"""System tests for management of qdrouter"""

import unittest, system_test, re
from qpid_dispatch_internal.management import Node, ManagementError
from system_test import Qdrouterd
from httplib import BAD_REQUEST, NOT_IMPLEMENTED

class ManagementTest(system_test.TestCase): # pylint: disable=too-many-public-methods

    @classmethod
    def setUpClass(cls):
        super(ManagementTest, cls).setUpClass()
        name = cls.__name__
        conf = Qdrouterd.Config([
            ('log', {'module':'DEFAULT', 'level':'trace', 'output':name+".log"}),
            ('router', {'mode': 'standalone', 'router-id': name}),
            ('listener', {'port':cls.get_port(), 'role':'normal'})
        ])
        cls.router = cls.tester.qdrouterd('%s'%name, conf)
        cls.router.wait_ready()

    def setUp(self):
        super(ManagementTest, self).setUp()
        self.node = self.cleanup(Node(self.router.addresses[0]))

    def assertRaisesManagement(self, status, pattern, call, *args, **kwargs):
        """Assert that call(*args, **kwargs) raises a ManagementError
        with status and matching pattern in description """
        try:
            call(*args, **kwargs)
            self.fail("Expected ManagementError with %s, %s"%(status, pattern))
        except ManagementError, e:
            self.assertEqual(e.status, status)
            assert re.search("(?i)"+pattern, e.description), "No match for %s in %s"%(pattern, e.description)

    def test_bad_query(self):
        """Test that various badly formed queries get the proper response"""
        self.assertRaisesManagement(
            BAD_REQUEST, "No operation", self.node.call, self.node.request())
        self.assertRaisesManagement(
            NOT_IMPLEMENTED, "Not Implemented: nosuch",
            self.node.call, self.node.request(operation="nosuch"))
        self.assertRaisesManagement(
            BAD_REQUEST, r'(entityType|attributeNames).*must be provided',
            self.node.query)

    def test_query_entity_type(self):
        address = 'org.apache.qpid.dispatch.router.address'
        response = self.node.query(entity_type=address)
        self.assertEqual(response.attribute_names[0:3], ['type', 'name', 'identity'])
        for r in response:  # Check types
            self.assertEqual(r.type, address)
        names = [r.name for r in response]
        self.assertTrue('L$management' in names)
        self.assertTrue('M0$management' in names)

        # TODO aconway 2014-06-05: negative test: offset, count not implemented on router
        try:
            # Try offset, count
            self.assertGreater(len(names), 2)
            response0 = self.node.query(entity_type=address, count=1)
            self.assertEqual(names[0:1], [r[1] for r in response0])
            response1_2 = self.node.query(entity_type=address, count=2, offset=1)
            self.assertEqual(names[1:3], [r[1] for r in response1_2])
            self.fail("Negative test passed!")
        except: pass

    def test_query_attribute_names(self):
        response = self.node.query(attribute_names=["type", "name", "identity"])
        # TODO aconway 2014-06-05: negative test: attribute_names query doesn't work.
        try:
            self.assertNotEqual([], response)
            self.fail("Negative test passed!")
        except: pass

if __name__ == '__main__':
    unittest.main()
