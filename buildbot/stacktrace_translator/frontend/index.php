<?php

/*
	Simple Frontend for the Stacktrace translator on http://springrts.com:8000/
	
	Note: the file lastrun has to be writeable by the webserver

*/


/*
	limits the usage of the script, only allow this script to be run every 10 seconds
*/
function limit(){
	$stat=stat("lastrun");
	$diff=$stat['mtime'] + 7 - time();
	if ( $diff > 0 )
		die("Please wait $diff seconds to rerun this script");
	else
		touch("lastrun");
}

/**
	returns the spring version of the string in filecontents
	for example
	Spring 0.82.5.0 (0.82.5)
*/
function getVersion($filecontents){
	$res=preg_match('/Spring [^\(]+ \(([^\)]+)\)/',$filecontents,$matches);
	if (($res==1) && (strpos($matches[1],"g")===false)){ // g means master version
		$pos=strpos($matches[1]," ");
		if ($pos===false)
			return $matches[1];
		return substr($matches[1],0,$pos);
	}
	return "master";
}
/**
	returns true if url is valid http:// url
	has to return false if url is local file
*/
function isValidURL($url){
	return preg_match('|^http(s)?://[a-z0-9-]+(.[a-z0-9-]+)*(:[0-9]+)?(/.*)?$|i', $url);
}
/**
	posts to pastebin, returns url
*/
function pastebin($text, $name=""){
	$name=substr($name,strlen($name)-23); //limit name to 23 chars
	$request = http_build_query(array( 'paste_code' => $text,
		'paste_subdomain' => 'spring-stacktrace',
		'paste_name' => $name));
	$context = stream_context_create(array('http' => array(
		'method' => "POST",
		'header' => 'Content-type: application/x-www-form-urlencoded',
		'content' => $request)));
	$file = file_get_contents("http://pastebin.com/api_public.php", false, $context);
	return $file;
}
/**
	does the translate_stacktrace request on $rpcserver and returns the decoded result
*/
function xmlrpcrequest($rpcserver, $infolog){
	$request = xmlrpc_encode_request("translate_stacktrace", $infolog);

	#$header[] = "Host: springrts.com";
	$header[] = "Content-type: text/xml";

	$curl=curl_init();
	curl_setopt( $curl, CURLOPT_URL, $rpcserver);
	curl_setopt( $curl, CURLOPT_RETURNTRANSFER, 1 );
	curl_setopt( $curl, CURLOPT_HTTPHEADER, $header );
	curl_setopt( $curl, CURLOPT_CUSTOMREQUEST, 'POST' );
	curl_setopt( $curl, CURLOPT_FAILONERROR, 1);
	curl_setopt( $curl, CURLOPT_POSTFIELDS, $request );

	$file=curl_exec($curl);
	$err=curl_error($curl);
	if ($file===false)
		die($rpcserver.":".$err);
	curl_close($curl);
	$res=xmlrpc_decode($file);
	return $res;
}

/**
	fetches infolog.txt and normalize it
*/
function getinfolog(){
	global $_REQUEST;
	if(array_key_exists('url',$_REQUEST)){
		$url=$_REQUEST['url'];
	}else
		$url="";
	if($url!=""){ //url parameter unset
		if (!isValidURL($url)){
			die("Invalid url!");
		}
		$infolog=file_get_contents($url,false, NULL, -1, 100000); //retrieve remote infolog.txt
	}else{
		if (array_key_exists('request',$_REQUEST))
			$infolog=$_REQUEST['request'];
		else
			return "";	
	}
	$infolog=str_replace("\r\n","\n",$infolog); //windows linebreaks f'up some things here...
	$infolog=str_replace("\n\n","\n",$infolog);
	return $infolog;
}

function parse_template($tpl, $vars){
	$file=file_get_contents($tpl);
	while(list($name,$value)=each($vars)){
		$file=str_replace("%".$name."%",$value,$file);
	}
	return $file;
}
/**
	parses the result of an xmlrequest and returns a string ready for html output
*/
function parse_result($res,$ver){
	$pastebin="";
	$name="";
	$textwithlinks="";
	if (array_key_exists('faultString',$res)){
		$cleantext= "Error: ".$res['faultString']."<br>\n";
		$cleantext.= "Maybe this stacktrace is from an self-compiled spring, or is the stack-trace to old?\n";
		$cleantext.= "This script only can handle >=0.82\n";
	}else{
		$textwithlinks="<h1>translated with links to github source ('$ver' detected)</h1>\n";
		$textwithlinks.="<table><tr><td>module</td><td>address</td><td>file</td><td>line</td></tr>\n";
		$cleantext="";
		for($i=0;$i<count($res); $i++){
			$module = $res[$i][0];
			$address = $res[$i][1];
			$filename = $res[$i][2];
			$line = $res[$i][3];
			if ($name=="")
				$name=$filename.":".$line;
			$textwithlinks.="<tr>\n";
			$textwithlinks.= "<td>".$module . "</td><td> " . $address . "</td><td> " . $filename . "</td>\n";
			$cleantext.= $module." ".$address." ".$filename.":".$line."\n";
			if ((strlen($filename)>0) && ($filename[0]=='r')){
				$textwithlinks.='<td><a target="_blank" href="http://github.com/spring/spring/tree/'.$ver.'/'.$filename.'#L'.$line.'">'.$line.'</a></td>';
			}else {
				$textwithlinks.="<td>$line</td>";
			}
			$textwithlinks.= "</tr>\n";
		}
		$textwithlinks.= "</table>\n";
	}
	if (($cleantext!="")&&(isset($_REQUEST['pastebin']))){
		$url=pastebin($cleantext,$name);
		$pastebin="Pastebin url: <a href=\"$url\" target=\"_blank\">$url</a>";
	}
	$cleantext = "<h1>translated for copy and paste</h1>\n<pre>$cleantext</pre>";

	return array( 'PASTEBIN' => $pastebin,
			'RESULTCLEAN' => $cleantext,
			'RESULTHTML' =>	$textwithlinks );
}
$res['PASTEBIN']=""; /*initianlize vars for template*/
$res['RESULTHTML']="";
$res['RESULTCLEAN']="";
$res['TEXTAREA']=getinfolog();
$res['TRANSLATOR']="http://springrts.com:8000";
$res['INFO']="";

if ($res['TEXTAREA']!=""){
	limit();
	$tmp=xmlrpcrequest($res['TRANSLATOR'],$res['TEXTAREA']);
/*	if (array_key_exists('faultString',$tmp)){
		$res['INFO']="<h1>Warning: using local translator, as remote can't translate</h1>";
		$res['TRANSLATOR']="http://abma.de:8000";
		$tmp=xmlrpcrequest($res['TRANSLATOR'],$res['TEXTAREA']);
	}*/
	$res=array_merge($res,parse_result($tmp,getVersion($res['TEXTAREA'])));
}
$res['ACTION']=$_SERVER['SCRIPT_NAME'];
echo parse_template("index.tpl",$res);

?>
