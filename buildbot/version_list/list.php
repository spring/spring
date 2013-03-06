<?php

/**
	lists all engine versions with metadata in json

*/

$allowed = array(
	'94.23.255.23', # springfiles.com
);

$ip = $_SERVER['REMOTE_ADDR'];
if (array_search($ip, $allowed) === FALSE) {
	die("Sorry, $ip isn't allowed to access this page");
}

$dirs = array(".");
$db = new SQLite3("fileattributes.sqlite3");
$db -> exec("PRAGMA journal_mode = TRUNCATE");
$db -> exec("CREATE TABLE IF NOT EXISTS files (filename STRING PRIMARY KEY, md5 STRING, filectime INT, filesize, INT)");

function getMD5($file){
	global $db;
	$res = $db->querySingle("SELECT md5, filectime, filesize FROM files WHERE filename='$file'", TRUE);
	if ($res==NULL) {
		$res = array();
		$res['md5'] = md5_file($file);
		$res['filectime'] = filectime($file);
		$res['filesize'] = filesize($file);
		$query = sprintf("INSERT INTO files (filename, md5, filectime, filesize) VALUES ('%s', '%s', %d, %d)",
			$file, $res['md5'], $res['filectime'], $res['filesize']);
		$db->exec($query);
	}
	return $res;
}

function getFileInfo($os, $regex, $path){
	$ret = array();
	if (preg_match($regex, $path, $res)) {
		$version = $res[1];
		if($version=="testing")
			return $ret;
		$ret['version'] = $version;
		$fileinfo = getMD5($path);
		$ret['filectime'] = $fileinfo['filectime'];
		$ret['filesize'] = $fileinfo['filesize'];
		$ret['md5'] = $fileinfo['md5'];
		$ret['path'] = $path;
		$ret['os'] = $os;
	}
	return $ret;
}

$res = array();
$regexes = array(
	"windows" => "/win32\/spring_(.*)_minimal-portable.7z$/",
	"macosx" => "/osx64\/[sS]pring_(.*)[_-]MacOSX-.*.zip$/",
	"linux" => "/linux32\/spring_(.*)_minimal-portable-linux32-static.7z$/",
	"linux64" => "/linux64\/spring_(.*)_minimal-portable-linux64-static.7z$/"
	);
while(count($dirs)>0) {
	$cur = array_pop($dirs);
	$dh = opendir($cur);
	while (false !== ($entry = readdir($dh))) {
		if ($entry[0]=='.')
			continue;
		if ($cur == ".")
			$next = $entry;
		else
			$next = $cur.'/'.$entry;
		if (is_dir($next)){
			$dirs[] = $next;
		} else {
			reset($regexes);
			while(list($os, $regex) = each($regexes)) {
				$arr = getFileInfo($os, $regex, $next);
				if (count($arr)>0)
					$res[] = $arr;
			}
		}
	}
	closedir($dh);
}

$db->close();
echo json_encode($res);

