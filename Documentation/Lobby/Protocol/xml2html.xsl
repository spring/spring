<?xml version="1.0" encoding="UTF-8"?>
<xsl:stylesheet version="1.0" xmlns:xsl="http://www.w3.org/1999/XSL/Transform">
<xsl:output method="html" omit-xml-declaration="yes" media-type="text/html" encoding="utf-8"/>

<!-- 

  See DTD file for more info on various elements/attributes!
   
  Some useful links:
  ******************
  
  How to test if node contains no text:
    http://www.dpawson.co.uk/xsl/sect2/N3328.html#d4281e58
    
  List of css tags:
    http://www.w3.org/TR/REC-CSS1

  CSS font-family:  
    http://www.codestyle.org/css/font-family/
 -->

<xsl:template match="ProtocolDescription">
  <html>
  <head>
    <title>Spring lobby protocol description</title>
    
    <style type="text/css">
      h3 {
      color: navy;
      font-size: 1.1em;
      line-height: 1.25em;
      text-align: left;
      margin-top: 0;
      margin-bottom: 0.25em;
      border-bottom: 1px solid navy
      }
      
    </style>
    
  </head>
  <body>
    <h1>Spring lobby protocol description</h1>
    
    <h2>Introduction</h2>
    <table border="0" style='width: 750px; table-layout: fixed; border: 2px dotted gray;'>
      <tr><td>
        <xsl:apply-templates select="Intro"/>
        <p>
        Some statistics on this document:<br />
        <ul>
        <li>Number of commands described: <xsl:value-of select="count(//Command)"/></li>
        <li>Number of client commands: <xsl:value-of select="count(//Command[@Source='client'])"/></li>
        <li>Number of server commands: <xsl:value-of select="count(//Command[@Source='server'])"/></li>
        </ul>
        </p>
      </td></tr>
    </table>  
    
    <xsl:if test="./RecentChanges">
      <h2>Recent changes</h2>
      <table border="0" style='width: 750px; table-layout: fixed; border: 2px dotted gray;'>
      <tr><td>
        <xsl:apply-templates select="RecentChanges"/>
      </td></tr>
      </table>  
    </xsl:if>
    
    <h2>Command list</h2>
   
    <xsl:for-each select="CommandList/Command">
    
      <xsl:variable name="headercolor">
        <xsl:choose>
          <xsl:when test="@Source='client'">#9ACD32</xsl:when>
          <xsl:otherwise>#E1EB72</xsl:otherwise>
        </xsl:choose>
      </xsl:variable>
      
      <xsl:variable name="commfull">
        <xsl:value-of select="@Name"/>
        <xsl:for-each select="Arguments/Argument">
          <xsl:choose>
            <xsl:when test="@Optional='yes' and @Sentence='yes'"><xsl:text> </xsl:text>[{<xsl:value-of select="@Name"/>}]</xsl:when>
            <xsl:when test="@Optional='no' and @Sentence='no'"><xsl:text> </xsl:text><xsl:value-of select="@Name"/></xsl:when>
            <xsl:when test="@Optional='yes' and @Sentence='no'"><xsl:text> </xsl:text>[<xsl:value-of select="@Name"/>]</xsl:when>
            <xsl:when test="@Optional='no' and @Sentence='yes'"><xsl:text> </xsl:text>{<xsl:value-of select="@Name"/>}</xsl:when>
          </xsl:choose>
        </xsl:for-each>
      </xsl:variable>
    
      <a name="{@Name}:{@Source}"/> <!-- we need this to reference it with "a href" tag -->
      <table border="0" style='width: 750px; table-layout: fixed; border: 2px dotted gray;'>
        <tr bgcolor="{$headercolor}">
          <td style='font-weight: bold; color: #4E5152'><xsl:value-of select="$commfull"/></td>
          <td style='width: 120px; text-align: center; font-style: italic'>
          Source: <xsl:value-of select="@Source"/>
          </td>
        </tr>
        <tr>
          <td colspan="2">
            <h3>Description</h3>
            <xsl:apply-templates select="Description" />
            <br />
            <br />
            
            <xsl:for-each select="Arguments/Argument">
              <xsl:if test="string(.)">
                <span style="color: blue; font-weight: bold; text-decoration: underline"><xsl:value-of select="@Name"/>:</span> <xsl:apply-templates select="." />
                <br />
                <br />
              </xsl:if>
            </xsl:for-each>
            
            <xsl:if test="string(Response)">
              <h3>Response</h3>
              <xsl:apply-templates select="Response" />
              <br />
              <br />
            </xsl:if>
            
            <xsl:if test="Examples/*">
              <h3>Examples</h3>
              <xsl:for-each select="Examples/Example">
		        <xsl:if test="string(.)">
                  <xsl:apply-templates select="."/>
                  <br />
		        </xsl:if>
              </xsl:for-each>
              <br />
            </xsl:if>
            
          </td>
        </tr>
      </table>
      <br />
      <br />      
    </xsl:for-each>       
    
  </body>
  </html>
</xsl:template>

<!-- this overrides default built-in template for text nodes. Currently doesn't do anything useful! -->
<xsl:template match="text()">
  <xsl:value-of select="(.)"/>
</xsl:template>

<xsl:template match="clink">

  <xsl:variable name="type"><xsl:value-of select="substring-after(@name, ':')"/></xsl:variable> <!-- doesn't neccessarily exist! -->
  
  <xsl:choose>
    <xsl:when test="$type=''">
      <!-- the ":x" post-fix doesn't exist -->
      <xsl:choose>
        <xsl:when test="//Command[@Name=current()/@name and @Source='client']">
          <a href="#{@name}:client"><xsl:value-of select="@name"/></a>
        </xsl:when>
        <xsl:when test="//Command[@Name=current()/@name and @Source='server']">
          <a href="#{@name}:server"><xsl:value-of select="@name"/></a>
        </xsl:when>
		<xsl:otherwise>
		  <!-- error: command does not exist! (link is broken) -->
          <a href="#{@name}"><xsl:value-of select="@name"/></a><span style="color: red"> [broken link]</span>
		</xsl:otherwise>        
      </xsl:choose>
    </xsl:when>
    <xsl:otherwise>
      <!-- the ":x" post-fix exists -->
      <xsl:choose>
        <xsl:when test="starts-with($type,'c')">
          <a href="#{substring-before(@name, ':')}:client"><xsl:value-of select="substring-before(@name, ':')"/></a>
        </xsl:when>
        <xsl:when test="starts-with($type,'s')">
          <a href="#{substring-before(@name, ':')}:server"><xsl:value-of select="substring-before(@name, ':')"/></a>
        </xsl:when>
		<xsl:otherwise>
		  <!-- error: command does not exist! (link is broken) -->
          <a href="#{@name}"><xsl:value-of select="@name"/></a><span style="color: red"> [broken link]</span>
		</xsl:otherwise>         
      </xsl:choose>
    </xsl:otherwise>
  </xsl:choose>

</xsl:template>

<xsl:template match="i">
  <span style="font-style: italic;"><xsl:apply-templates /></span>
</xsl:template>

<xsl:template match="b">
  <span style="font-weight: bold;"><xsl:apply-templates /></span>
</xsl:template>

<xsl:template match="u">
  <span style="text-decoration: underline;"><xsl:apply-templates /></span>
</xsl:template>

<xsl:template match="br">
  <br /><xsl:apply-templates />
</xsl:template>

<xsl:template match="br2">
  <br /><br /><xsl:apply-templates />
</xsl:template>

<xsl:template match="p">
  <p><xsl:apply-templates /></p>
</xsl:template>

<xsl:template match="ul">
  <ul><xsl:apply-templates /></ul>
</xsl:template>

<xsl:template match="li">
  <li><xsl:apply-templates /></li>
</xsl:template>

<xsl:template match="url">
  <xsl:variable name="link"><xsl:value-of select="." /></xsl:variable>
  <a href="{$link}" style="white-space: nowrap"><xsl:value-of select="$link" /></a>
</xsl:template>

<!-- This template replaces all newlines with html BR tags 
     Code has been copied from: 
     http://www.biglist.com/lists/xsl-list/archives/200310/msg01013.html
-->
<xsl:template name="insertBreaks">
   <xsl:param name="text" select="."/>
   <xsl:choose>
   <xsl:when test="contains($text, '&#xa;')">
      <xsl:value-of select="substring-before($text, '&#xa;')"/>
      <br />
      <xsl:call-template name="insertBreaks">
          <xsl:with-param name="text" select="substring-after($text,'&#xa;')"/>
      </xsl:call-template>
   </xsl:when>
   <xsl:otherwise>
	<xsl:value-of select="$text"/>
   </xsl:otherwise>
   </xsl:choose>
</xsl:template>

</xsl:stylesheet>