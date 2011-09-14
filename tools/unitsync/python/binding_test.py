#! /usr/bin/env python

import pyunitsync as unitsync
import unittest, random,string,os,sys,tempfile

testRangeSwitch = None

try:
	from PIL import Image
	import numpy
	testMinimap = True
except:
	testMinimap = False

def testRange(count):
	if testRangeSwitch == 0:
		return [0]
	elif testRangeSwitch == 1:
		return range(count)
	else:
		return [random.randint(0,count-1)]

class UnitsyncTestCase(unittest.TestCase):
	def setUp(self):
		#redirect is not working, why?
		#sys.stderr = sys.stdout=open('/dev/null','wb')
		unitsync.Init(True,0)
	def tearDown(self):
		unitsync.UnInit()
		#sys.stdout=sys.__stdout__
		#sys.stderr=sys.__stderr__
		
	def infotest(self, infocount):
		infofuncs = { "string":"GetInfoValueString", "integer":"GetInfoValueInteger", "float":"GetInfoValueFloat", "bool":"GetInfoValueBool" }
		for i in range(infocount):
			key = unitsync.GetInfoKey(i)
			self.assertTrue( None != key )
			infotype = unitsync.GetInfoType( i )
			self.assertTrue( None != infotype )
			self.assertTrue( infotype in infofuncs.keys() )
			value = getattr( unitsync, infofuncs[infotype] )(i)
			self.assertTrue( None != value )
			
	def optiontest(self, optioncount):
		optionfuncs = { 1:('GetOptionBoolDef','GetOptionBoolDef'),
			2:('GetOptionListCount','GetOptionListDef','GetOptionListItemKey','GetOptionListItemName','GetOptionListItemDesc'),
			3:('GetOptionNumberDef','GetOptionNumberMin','GetOptionNumberMax','GetOptionNumberStep'),
			4:('GetOptionStringDef','GetOptionStringMaxLen'),
			5:[] }# shouldn't there be sth about sections too?? 

		#these just have to be not NULL
		common_funcs = ('GetOptionKey','GetOptionScope','GetOptionName','GetOptionSection','GetOptionStyle','GetOptionDesc')
		for i in range(optioncount):
			opttype = unitsync.GetOptionType(i)
			self.assertTrue( 0 < opttype < 6 )
			for func in common_funcs:
				self.assertTrue( None != getattr( unitsync, func )(i) )
			for func in optionfuncs[opttype]:
				func = getattr( unitsync, func )
				#assure func is wrapped
				self.assertTrue( None != func )
				#try to call those funcs with one single index arg
				try:
					value = func(i)
					self.assertTrue( None != value )
				except TypeError:
					#fail w/o consueq
					pass

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
		for mapidx in testRange(mapcount):
			mapname = unitsync.GetMapName(mapidx)
			self.assertTrue(None != mapname )
			for func in ['GetMapWidth' ,'GetMapHeight','GetMapTidalStrength','GetMapWindMin','GetMapWindMax',
				'GetMapGravity','GetMapResourceCount', 'GetMapPosCount']:
				self.assertTrue( -1 < getattr(unitsync,func)(mapidx) )
			for func in ['GetMapName','GetMapFileName','GetMapDescription','GetMapAuthor']:
				self.assertTrue( None != getattr(unitsync,func)(mapidx) )
			mapoptcount = unitsync.GetMapOptionCount( mapname )
			self.optiontest( mapoptcount )

	def test_map_archives(self):
		mapcount = unitsync.GetMapCount()
		if self._no_maps():
			return
		for mapidx in testRange(mapcount):
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
		for mapidx in testRange(mapcount):
			checksum = unitsync.GetMapChecksum( mapidx )
			self.assertTrue( checksum > 0 )
			self.assertEqual( checksum, unitsync.GetMapChecksumFromName( unitsync.GetMapName(mapidx) ) )
			
	def test_minimap(self):
		mapcount = unitsync.GetMapCount()
		if self._no_maps():
			return
		for mapidx in testRange(mapcount):
			for miplevel in range(9):#max miplevel is 8
				#miplevel = 2
				mapname = unitsync.GetMapName( mapidx )
				minimap = unitsync.GetMinimap( mapname, miplevel )
				self.assertTrue(None!=minimap)
				if testMinimap:
					size = ( 1 << ( 10 - miplevel ), 1 << ( 10 - miplevel ) )
					arr = numpy.frombuffer(minimap, numpy.uint16).astype(numpy.uint32)
					arr = 0xFF000000 + ((arr & 0xF800) >> 8 ) + ((arr & 0x07E0) << 5) + ((arr & 0x001F) << 19)
					image = Image.frombuffer('RGBA', size, arr, 'raw', 'RGBA', 0,1)
					image.save( os.path.join(  tempfile.gettempdir(), 'pyusync_%s_mip-%02d.png'%(mapname,miplevel) ) )


class ModTest(UnitsyncTestCase):
	checked_count = False
	
	def _no_mods(self):
		modcount = unitsync.GetPrimaryModCount()
		if modcount == 0:
			if not self.checked_count:
				sys.stderr.write('No mods found, skipping extended mod function checks\n')
		self.checked_count = True
		return modcount == 0
		
	def test_mod_info(self):
		modcount = unitsync.GetPrimaryModCount()
		self.assertEqual(None,unitsync.GetPrimaryModName(modcount))
		if self._no_mods():
			return
		for modindex in testRange(modcount):
			modname = unitsync.GetPrimaryModName(modindex)
			self.assertTrue(None!=modname)
			for func in ['GetPrimaryModShortName', 'GetPrimaryModVersion', 
				'GetPrimaryModMutator', 'GetPrimaryModGame', 'GetPrimaryModShortGame', 
				'GetPrimaryModDescription']:
				self.assertTrue( None != getattr(unitsync,func)(modindex) )
			infocount = unitsync.GetPrimaryModInfoCount(modindex)
			self.assertTrue( 0 < infocount )
			self.infotest( infocount )

	def test_mod_archives(self):
		#add archives
		modcount = unitsync.GetPrimaryModCount()
		if self._no_mods():
			return
		for modidx in testRange(modcount):
			modname = unitsync.GetPrimaryModName(modidx)
			self.assertTrue(unitsync.GetPrimaryModArchiveCount(modidx))
			archivename = unitsync.GetPrimaryModArchive( modidx )
			self.assertTrue( None != archivename )
			unitsync.AddAllArchives( archivename );
			modoptcount = unitsync.GetModOptionCount()
			self.optiontest( modoptcount )
			unitsync.RemoveAllArchives()
	
class AiTest(UnitsyncTestCase):
	checked_count = False
	
	def _no_ais(self):
		aicount = unitsync.GetSkirmishAICount()
		if aicount == 0:
			if not self.checked_count:
				sys.stderr.write('No ais found, skipping extended ai function checks\n')
		self.checked_count = True
		return aicount == 0
		
	def test_ai_info(self):
		aicount = unitsync.GetPrimaryModCount()
		self.assertEqual(None,unitsync.GetPrimaryModName(aicount))
		if self._no_ais():
			return
		for aiindex in testRange(aicount):
			infocount = unitsync.GetSkirmishAIInfoCount(aiindex)
			self.assertTrue( 0 <= infocount )#docs say 0 -> error though
			self.infotest( infocount )
			optcount = unitsync.GetSkirmishAIOptionCount( aiindex )
			self.assertTrue( 0 <= optcount ) #docs say 0 -> error though
			self.optiontest( optcount )
		
	
if __name__ == '__main__':
	try:
		testRangeSwitch = int ( sys.argv[1] )
	except:
		testRangeSwitch = 0
	basicSuite = unittest.TestLoader().loadTestsFromTestCase(BasicTest)
	mapSuite = unittest.TestLoader().loadTestsFromTestCase(MapTest)
	modSuite = unittest.TestLoader().loadTestsFromTestCase(ModTest)
	aiSuite = unittest.TestLoader().loadTestsFromTestCase(AiTest)
	unittest.TextTestRunner(verbosity=4).run(unittest.TestSuite([basicSuite,mapSuite,modSuite,aiSuite]))
	#unittest.TextTestRunner(verbosity=4).run(unittest.TestSuite(mapSuite))
 
