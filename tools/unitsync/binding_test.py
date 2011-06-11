#! /usr/bin/env python

import pyunitsync as unitsync
import unittest, random,string,os,sys

class UnitsyncTestCase(unittest.TestCase):
	def setUp(self):
		#redirect is not working, why?
		#sys.stderr = sys.stdout=open('/dev/null','wb')
		unitsync.Init(True,0)
	def tearDown(self):
		unitsync.UnInit()
		#sys.stdout=sys.__stdout__
		#sys.stderr=sys.__stderr__
		
class BasicTest(UnitsyncTestCase):
	def test_next_error(self):
		self.assertEqual(None,unitsync.GetNextError())
		
	def test_datadir(self):
		self.assertTrue(unitsync.GetDataDirectoryCount()>=0)
		self.assertTrue(unitsync.GetWritableDataDirectory()!=None)
	
	def test_config_key(self):
		def random_key():
			return ''.join(random.choice(string.ascii_uppercase + string.digits) for x in range(20))
		eps = 10e-7#get the maximum theoretical conv error instead
		self.assertTrue(abs(66.6-unitsync.GetSpringConfigFloat(random_key(),66.6))< (66.6*eps))
		self.assertEqual("Supercalifragilisticexpialidocious",unitsync.GetSpringConfigString(random_key(),"Supercalifragilisticexpialidocious"))
		self.assertEqual(42,unitsync.GetSpringConfigInt(random_key(),42))
	
class MapTest(UnitsyncTestCase):
	checked_count = False
	
	def _no_maps(self):
		mapcount = unitsync.GetMapCount()
		if mapcount == 0:
			if not self.checked_count:
				sys.stderr.write('No maps found, skipping extended map function checks\n')
		self.checked_count = True
		return mapcount == 0

	def test_map_attributes(self):
		mapcount = unitsync.GetMapCount()
		self.assertEqual(None,unitsync.GetMapName(mapcount))
		if self._no_maps():
			return
		self.assertTrue(None != unitsync.GetMapName(mapcount-1))
		mapidx = 0
		for func in ['GetMapWidth' ,'GetMapHeight','GetMapTidalStrength','GetMapWindMin','GetMapWindMax',
			'GetMapGravity','GetMapResourceCount', 'GetMapPosCount']:
			self.assertTrue( -1 < getattr(unitsync,func)(mapidx) )

	def test_map_archives(self):
		mapcount = unitsync.GetMapCount()
		if self._no_maps():
			return
		mapidx = 0
		mapname = unitsync.GetMapName(mapidx)
		self.assertTrue(unitsync.GetMapArchiveCount(mapname))
		archivename = unitsync.GetMapArchiveName(0)
		self.assertTrue( None != archivename )
		unitsync.AddArchive( archivename )
		self.assertTrue( unitsync.OpenFileVFS( archivename ) )
		unitsync.RemoveAllArchives()
		
	def test_map_checksum(self):
		mapcount = unitsync.GetMapCount()
		if self._no_maps():
			return
		checksum = unitsync.GetMapChecksum( mapcount -1 )
		self.assertTrue( checksum > 0 )
		self.assertEqual( checksum, unitsync.GetMapChecksumFromName( unitsync.GetMapName(mapcount -1) ) )
		
	
if __name__ == '__main__':
	basicSuite = unittest.TestLoader().loadTestsFromTestCase(BasicTest)
	mapSuite = unittest.TestLoader().loadTestsFromTestCase(MapTest)
	unittest.TextTestRunner(verbosity=4).run(unittest.TestSuite([basicSuite,mapSuite]))
 
