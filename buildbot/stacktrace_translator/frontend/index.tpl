<!DOCTYPE HTML PUBLIC "-//W3C//DTD HTML 4.01//EN"
   "http://www.w3.org/TR/html4/strict.dtd">
<html>
<head>
	<title>Spring RTS - stacktrace translator frontend</title>
	<link type="text/css" rel="stylesheet" media="all" href="style.css">
	<meta http-equiv="Content-Type" content="text/html; charset=utf-8">
</head>
<body>
<p>
This page is a simple frontend for <a href="%TRANSLATOR%">%TRANSLATOR%</a><br>
Use a url or copy and paste to this Form to translate the infolog.txt of a crashed spring. <br>
At max 100.000 Bytes are downloaded!<br>
<b>Note: only windows-versions can be translated.</b>
</p>

<form method="post" action="%ACTION%"><p>Url of <b>unformatted raw plaintext</b> infolog.txt
<input type="text" name="url" size="40">
or copy and paste here:
<br>
<textarea cols="100" rows="20" name="request">%TEXTAREA%</textarea><br>
Post to <a target="_blank" href="http://paste.springfiles.com">paste.springfiles.com</a><input type="checkbox" name="pastebin">
<input type="submit">
</p>
</form>
	%INFO%
	%PASTEBIN%
	%RESULTHTML%
	%RESULTCLEAN%
	</body>
</html>
