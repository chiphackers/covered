<xsl:stylesheet xmlns:xsl="http://www.w3.org/1999/XSL/Transform"
                xmlns:exsl="http://exslt.org/common"
                xmlns:cf="http://docbook.sourceforge.net/xmlns/chunkfast/1.0"
                version="1.0"
                exclude-result-prefixes="cf exsl">
  
<!-- ********************************************************************
     $Id$
     ********************************************************************

     This file is part of the XSL DocBook Stylesheet distribution.
     See ../README or http://nwalsh.com/docbook/xsl/ for copyright
     and other information.

     ******************************************************************** -->

<!-- ==================================================================== -->

  <xsl:import href="chunkfast.xsl"/>

  <!-- Set parameters -->
  <xsl:param name="draft.mode" select="'no'"/>
  <xsl:param name="section.autolabel" select="1"/>
  <xsl:param name="section.autolabel.max.depth" select="1"/>
  <xsl:param name="section.label.includes.component.label" select="1"/>
  <xsl:param name="use.extensions" select="0"/>
  <xsl:param name="chunk.section.depth" select="0"/>
  <xsl:param name="chunk.first.sections" select="1"/>
  <xsl:param name="use.id.as.filename" select="1"/>
  <xsl:param name="admon.graphics.path" select="'img/'"/>
  <xsl:param name="admon.graphics.extension" select="'.gif'"/>
  <xsl:param name="admon.graphics" select="1"/>
  <xsl:param name="navig.graphics.path" select="'img/'"/>
  <xsl:param name="navig.graphics.extension" select="'.gif'"/>
  <xsl:param name="navig.graphics" select="1"/>
  <xsl:param name="base.dir" select="'../html/'"/>
  <xsl:param name="toc.max.depth" select="2"/>
  <xsl:param name="html.stylesheet" select="'covered.css'"/>
  <xsl:param name="header.image.filename" select="'img/banner.jpg'"/>
  <xsl:param name="region.before.extent" select="100"/>

  <!-- Customized templates -->
  <xsl:template name="body.attributes">
    <xsl:attribute name="bgcolor">#dfeef8</xsl:attribute>
    <xsl:attribute name="text">black</xsl:attribute>
    <xsl:attribute name="link">#0000FF</xsl:attribute>
    <xsl:attribute name="vlink">#840084</xsl:attribute>
    <xsl:attribute name="alink">#0000FF</xsl:attribute>
  </xsl:template>

  <!-- Customize each page with the Covered banner -->
  <xsl:template name="user.head.content">
    <center><img src="img/banner.jpg"/></center>
    <hr/>
  </xsl:template>

</xsl:stylesheet>
