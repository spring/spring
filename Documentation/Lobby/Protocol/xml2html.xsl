<?xml version="1.0" encoding="UTF-8"?>
<xsl:stylesheet version="1.0" xmlns:xsl="http://www.w3.org/1999/XSL/Transform">
<xsl:output method="html" omit-xml-declaration="yes" media-type="text/html" encoding="utf-8" indent="yes"/>

<!-- 
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
            <xsl:when test="@Optional='yes' and @Sentence='yes'"> [{<xsl:value-of select="@Name"/>}]</xsl:when>
            <xsl:when test="@Optional='no' and @Sentence='no'">&#x20;<xsl:value-of select="@Name"/></xsl:when><xsl:when test="@Optional=yes and @Sentence=yes">[{<xsl:value-of select="@Name"/>}]</xsl:when>
            <xsl:when test="@Optional='yes' and @Sentence='no'"> [<xsl:value-of select="@Name"/>]</xsl:when>
            <xsl:when test="@Optional='no' and @Sentence='yes'"> {<xsl:value-of select="@Name"/>}</xsl:when>
          </xsl:choose>
        </xsl:for-each>
      </xsl:variable>
    
      <table border="0" style='width: 750px; table-layout: fixed; border: 2px dashed gray;'>
        <tr bgcolor="{$headercolor}">
          <a name="{@Name}"/> <!-- we need this to reference it with "a href" tag -->
          <td style='font-weight: bold; color: #4E5152'><xsl:value-of select="$commfull"/></td>
          <td style='width: 120px; text-align: center; font-style: italic'>
          Source: <xsl:value-of select="@Source"/>
          </td>
        </tr>
        <tr>
          <td colspan="2">
            <h3>Description</h3>
            <xsl:value-of select="Description"/>
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
            
            <xsl:if test="string(Examples)">
              <h3>Usage examples</h3>
  
              <span style='white-space:pre;'><xsl:apply-templates select="Examples"/></span>
              
              <br />
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

<xsl:template name="commontext_handler">
  <span style='white-space:pre'><xsl:apply-templates /></span>
</xsl:template>

<xsl:template match="clink">
  <a href="#{@name}"><xsl:value-of select="@name"/></a>
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

</xsl:stylesheet>