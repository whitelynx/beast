<?xml version="1.0"?>
<xsl:stylesheet xmlns:xsl="http://www.w3.org/1999/XSL/Transform" version="1.0">
<xsl:output method="text" indent="no" charset="UTF-8"/>
<xsl:strip-space elements="*"/>
<xsl:preserve-space elements="keepspace preformat programlisting reference-scheme"/>

<xsl:param name="revision"/>
<xsl:param name="man_section"/>

<xsl:template match="texinfo">.\" t
.TH "<xsl:value-of select="settitle"/>" "<xsl:value-of select="$man_section"/>" "<xsl:value-of select="$revision"/>" "<xsl:value-of select="para/document-package"/>" "<xsl:value-of select="para/document-package"/>"
<xsl:apply-templates/>
</xsl:template>

<xsl:template match="setfilename|settitle|document-title|document-author|document-package|itemfunction|columnfraction"/>

<!--
<xsl:template match="para/revision">
  <xsl:choose>
    <xsl:when test="string-length($revision) > 0">
      <xsl:text>Document revised: </xsl:text><xsl:value-of select="$revision"/><xsl:text>
</xsl:text>
    </xsl:when>
    <xsl:otherwise>
      <xsl:message>XSL-WARNING: Skipping Document Revision line, revision date not provided.</xsl:message>
    </xsl:otherwise>
  </xsl:choose>
</xsl:template>
-->

<!-- Alper: fix this template by removing para tags when makeinfo is fixed -->
<xsl:template match="para/table-of-contents"><xsl:text>.SS Table of Contents
</xsl:text>
  <xsl:for-each select="/texinfo/chapter|/texinfo/unnumbered|/texinfo/appendix">
    <xsl:if test="local-name() = 'chapter'">
      <xsl:call-template name="toc_chapter"/>
    </xsl:if>
    <xsl:if test="local-name() = 'unnumbered'">
      <xsl:call-template name="toc_unnumbered"/>
    </xsl:if>
    <xsl:if test="local-name() = 'appendix'">
      <xsl:call-template name="toc_appendix"/>
    </xsl:if>
  </xsl:for-each>
</xsl:template>

<xsl:template name="toc_chapter"><xsl:number format="1 - "/><xsl:value-of select="title"/>
  <xsl:choose>
    <xsl:when test="count(./section) > 0">
<xsl:text>
.RS 2
</xsl:text>
    <xsl:for-each select="./section">
      <xsl:call-template name="toc_section"/>
    </xsl:for-each>
<xsl:text>
.RE 2
</xsl:text>
    </xsl:when>
    <xsl:otherwise><xsl:text>
.br
</xsl:text>
    </xsl:otherwise>
  </xsl:choose>
</xsl:template>

<xsl:template name="toc_section"><xsl:number level="multiple" count="chapter|section" format="1.1 - "/><xsl:value-of select="title"/>
  <xsl:choose>
    <xsl:when test="count(./subsection) > 0">
<xsl:text>
.RS 2
</xsl:text>
    <xsl:for-each select="./subsection">
      <xsl:call-template name="toc_subsection"/>
    </xsl:for-each>
<xsl:text>
.RE 2
</xsl:text>
    </xsl:when>
    <xsl:otherwise><xsl:text>
.br
</xsl:text>
    </xsl:otherwise>
  </xsl:choose>
</xsl:template>

<xsl:template name="toc_subsection"><xsl:number level="multiple" count="chapter|section|subsection" format="1.1.1 - "/><xsl:value-of select="title"/>
  <xsl:choose>
    <xsl:when test="count(./subsubsection) > 0">
<xsl:text>
.RS 2
</xsl:text>
    <xsl:for-each select="./subsubsection">
      <xsl:call-template name="toc_subsubsection"/>
    </xsl:for-each>
<xsl:text>
.RE 2
</xsl:text>
    </xsl:when>
    <xsl:otherwise><xsl:text>
.br
</xsl:text>
    </xsl:otherwise>
  </xsl:choose>
</xsl:template>

<xsl:template name="toc_subsubsection"><xsl:number level="multiple" count="chapter|section|subsection|subsubsection" format="1.1.1.1 - "/><xsl:value-of select="title"/><xsl:text>
.br
</xsl:text></xsl:template>

<xsl:template name="toc_appendix"><xsl:text>Appendix </xsl:text><xsl:number format="A - "/><xsl:value-of select="title"/>
  <xsl:choose>
    <xsl:when test="count(./appendixsec) > 0">
<xsl:text>
.RS 2
</xsl:text>
    <xsl:for-each select="./appendixsec">
      <xsl:call-template name="toc_appendixsec"/>
    </xsl:for-each>
<xsl:text>
.RE 2
</xsl:text>
    </xsl:when>
    <xsl:otherwise><xsl:text>
.br
</xsl:text>
    </xsl:otherwise>
  </xsl:choose>
</xsl:template>

<xsl:template name="toc_appendixsec"><xsl:number level="multiple" count="appendix|appendixsec" format="A.1 - "/><xsl:value-of select="title"/>
  <xsl:choose>
    <xsl:when test="count(./appendixsubsec) > 0">
<xsl:text>
.RS 2
</xsl:text>
    <xsl:for-each select="./appendixsubsec">
      <xsl:call-template name="toc_appendixsubsec"/>
    </xsl:for-each>
<xsl:text>
.RE 2
</xsl:text>
    </xsl:when>
    <xsl:otherwise><xsl:text>
.br
</xsl:text>
    </xsl:otherwise>
  </xsl:choose>
</xsl:template>

<xsl:template name="toc_appendixsubsec"><xsl:number level="multiple" count="appendix|appendixsec|appendixsubsec" format="A.1.1 - "/><xsl:value-of select="title"/>
  <xsl:choose>
    <xsl:when test="count(./appendixsubsubsec) > 0">
<xsl:text>
.RS 2
</xsl:text>
    <xsl:for-each select="./appendixsubsubsec">
      <xsl:call-template name="toc_appendixsubsubsec"/>
    </xsl:for-each>
<xsl:text>
.RE 2
</xsl:text>
    </xsl:when>
    <xsl:otherwise><xsl:text>
.br
</xsl:text>
    </xsl:otherwise>
  </xsl:choose>
</xsl:template>

<xsl:template name="toc_appendixsubsubsec"><xsl:number level="multiple" count="appendix|appendixsec|appendixsubsec|appendixsubsubsec" format="A.1.1.1 - "/><xsl:value-of select="title"/><xsl:text>
.br
</xsl:text></xsl:template>

<xsl:template name="toc_unnumbered"><xsl:value-of select="title"/>
  <xsl:choose>
    <xsl:when test="count(./unnumberedsec) > 0">
<xsl:text>
.RS 2
</xsl:text>
    <xsl:for-each select="./unnumberedsec">
      <xsl:call-template name="toc_unnumberedsec"/>
    </xsl:for-each>
<xsl:text>
.RE 2
</xsl:text>
    </xsl:when>
    <xsl:otherwise><xsl:text>
.br
</xsl:text>
    </xsl:otherwise>
  </xsl:choose>
</xsl:template>

<xsl:template name="toc_unnumberedsec"><xsl:value-of select="title"/>
  <xsl:choose>
    <xsl:when test="count(./unnumberedsubsec) > 0">
<xsl:text>
.RS 2
</xsl:text>
    <xsl:for-each select="./unnumberedsubsec">
      <xsl:call-template name="toc_unnumberedsubsec"/>
    </xsl:for-each>
<xsl:text>
.RE 2
</xsl:text>
    </xsl:when>
    <xsl:otherwise><xsl:text>
.br
</xsl:text>
    </xsl:otherwise>
  </xsl:choose>
</xsl:template>

<xsl:template name="toc_unnumberedsubsec"><xsl:value-of select="title"/>
  <xsl:choose>
    <xsl:when test="count(./unnumberedsubsubsec) > 0">
<xsl:text>
.RS 2
</xsl:text>
    <xsl:for-each select="./unnumberedsubsubsec">
      <xsl:call-template name="toc_unnumberedsubsubsec"/>
    </xsl:for-each>
<xsl:text>
.RE 2
</xsl:text>
    </xsl:when>
    <xsl:otherwise><xsl:text>
.br
</xsl:text>
    </xsl:otherwise>
  </xsl:choose>
</xsl:template>

<xsl:template name="toc_unnumberedsubsubsec"><xsl:value-of select="title"/><xsl:text>
.br
</xsl:text></xsl:template>

<xsl:template match="chapter/title">.SH <xsl:number count="chapter" format="1 - "/><xsl:apply-templates/><xsl:text>
</xsl:text></xsl:template>

<xsl:template match="section/title">.SS <xsl:number level="multiple" count="chapter|section" format="1.1 - "/><xsl:apply-templates/><xsl:text>
</xsl:text></xsl:template>

<xsl:template match="subsection/title"> <xsl:number level="multiple" count="chapter|section|subsection" format="1.1.1 - "/><xsl:apply-templates/><xsl:text>
</xsl:text></xsl:template>

<xsl:template match="subsubsection/title"> <xsl:number level="multiple" count="chapter|section|subsection|subsubsection" format="1.1.1.1 - "/><xsl:apply-templates/><xsl:text>
</xsl:text></xsl:template>

<xsl:template match="appendix/title">.SH <xsl:number count="appendix" format="A - "/><xsl:apply-templates/><xsl:text>
</xsl:text></xsl:template>

<xsl:template match="appendixsec/title">.SS <xsl:number level="multiple" count="appendix|appendixsec" format="A.1 - "/><xsl:apply-templates/><xsl:text>
</xsl:text></xsl:template>

<xsl:template match="appendixsubsec/title"> <xsl:number level="multiple" count="appendix|appendixsec|appendixsubsec" format="A.1.1 - "/><xsl:apply-templates/><xsl:text>
</xsl:text></xsl:template>

<xsl:template match="appendixsubsubsec/title"> <xsl:number level="multiple" count="appendix|appendixsec|appendixsubsec|appendixsubsubsec" format="A.1.1.1 - "/><xsl:apply-templates/><xsl:text>
</xsl:text></xsl:template>

<xsl:template match="unnumbered/title">.SH <xsl:apply-templates/><xsl:text>
</xsl:text>
</xsl:template>

<xsl:template match="unnumberedsec/title">.SS <xsl:apply-templates/><xsl:text>
</xsl:text></xsl:template>

<xsl:template match="unnumberedsubsec/title"> <xsl:apply-templates/><xsl:text>
</xsl:text></xsl:template>

<xsl:template match="unnumberedsubsubsec/title"> <xsl:apply-templates/><xsl:text>
</xsl:text></xsl:template>

<xsl:template match="chapheading/title|majorheading/title">.SH <xsl:apply-templates/><xsl:text>
</xsl:text></xsl:template>

<xsl:template match="heading/title">.SS <xsl:apply-templates/><xsl:text>
</xsl:text></xsl:template>

<xsl:template match="subheading/title"> <xsl:apply-templates/><xsl:text>
</xsl:text></xsl:template>

<xsl:template match="subsubheading/title"> <xsl:apply-templates/><xsl:text>
</xsl:text></xsl:template>

<xsl:template match="enumerate|itemize"><xsl:text>
.RS 2
</xsl:text><xsl:apply-templates/><xsl:text>
.RE 2
</xsl:text></xsl:template>

<xsl:template match="enumerate/item"><xsl:text>\(bu </xsl:text><xsl:apply-templates/><xsl:text>
</xsl:text></xsl:template>

<xsl:template match="itemize/item"><xsl:number format="1."/> <xsl:apply-templates/><xsl:text>
</xsl:text></xsl:template>

<xsl:template match="multitable">.TS
nokeep;
l l l.
<xsl:apply-templates/>
.TE
</xsl:template>

<xsl:template match="multitable/row"><xsl:apply-templates/><xsl:if test="not(position() = last())"><xsl:text>
</xsl:text></xsl:if></xsl:template>

<xsl:template match="multitable/row/entry"><xsl:apply-templates/><xsl:if test="not(position() = last())"><xsl:text>	</xsl:text></xsl:if></xsl:template>

<xsl:template match="table">
  <dl>
    <xsl:apply-templates/>
  </dl>
</xsl:template>

<xsl:template match="tableterm">
  <dt>
    <xsl:apply-templates/>
  </dt>
</xsl:template>

<xsl:template match="tableitem/item/para">
  <dd>
    <xsl:apply-templates/>
  </dd>
</xsl:template>

<xsl:template match="para">
  <xsl:choose>
    <!-- If this para is the parent of a revision or toc tag, then we -->
    <!-- omit the <p> tag in output -->
    <xsl:when test="(count(./revision) + count(./table-of-contents) + count(./document-author) + count(./document-title) + count(./document-package) + count(./printplainindex) + count(./reference-docname) + count(./reference-title) + count(./reference-scheme) + count(./reference-function) + count(./reference-parameter) + count(./reference-type) + count(./reference-returns) + count(./reference-blurb) + count(./reference-struct-name) + count(./reference-struct-open) + count(./reference-struct-close)) > 0"><xsl:apply-templates/><xsl:text>
</xsl:text></xsl:when>
    <!-- Paragrapgh is bogus (ie. white-space only), skip it -->
    <xsl:when test="normalize-space(.) = ''"/>
    <xsl:otherwise><xsl:text>.PP
</xsl:text><xsl:apply-templates/><xsl:text>
</xsl:text>
    </xsl:otherwise>
  </xsl:choose>
</xsl:template>

<xsl:template match="uref">
  <a>
    <xsl:attribute name="href">
      <xsl:if test="contains(urefurl, '@') and not(contains(substring-before(urefurl, '@'),':'))">
	<xsl:text>mailto:</xsl:text>
      </xsl:if><xsl:value-of select="urefurl"/>
    </xsl:attribute>
    <xsl:choose>
      <xsl:when test="count(child::urefreplacement)"><xsl:apply-templates select="urefreplacement"/></xsl:when>
      <xsl:when test="count(child::urefdesc)"><xsl:apply-templates select="urefdesc"/> (<xsl:apply-templates select="urefurl"/>)</xsl:when>
      <xsl:otherwise><xsl:apply-templates select="urefurl"/></xsl:otherwise>
    </xsl:choose>
  </a>
</xsl:template>

<xsl:template match="code">
  <code><xsl:apply-templates/></code>
</xsl:template>

<xsl:template match="kbd">
  <kbd><xsl:apply-templates/></kbd>
</xsl:template>

<xsl:template match="samp">
  <samp><xsl:apply-templates/></samp>
</xsl:template>

<xsl:template match="var">
  <var><xsl:apply-templates/></var>
</xsl:template>

<xsl:template match="dfn">
  <dfn><xsl:apply-templates/></dfn>
</xsl:template>

<xsl:template match="cite">
  <cite><xsl:apply-templates/></cite>
</xsl:template>

<xsl:template match="acronym">
  <acronym><xsl:apply-templates/></acronym>
</xsl:template>

<xsl:template match="url|email|key|env|file|command|option">
  <code>
    <span>
      <xsl:attribute name="class">
        <xsl:value-of select="local-name()"/>
      </xsl:attribute>
      <xsl:apply-templates/>
    </span>
  </code>
</xsl:template>

<xsl:template match="programlisting">
  <pre class="programlisting"><xsl:apply-templates/></pre>
</xsl:template>

<xsl:template match="menupath">
  <strong><span class="menupath"><xsl:apply-templates/></span></strong>
</xsl:template>

<xsl:template match="pagepath">
  <strong><span class="pagepath"><xsl:apply-templates/></span></strong>
</xsl:template>

<xsl:template match="property">
  "<em><span class="property"><xsl:apply-templates/></span></em>"
</xsl:template>

<xsl:template match="channel">
  <em><span class="channel"><xsl:apply-templates/></span></em>
</xsl:template>

<xsl:template match="object">
  <em><code><span class="object"><xsl:apply-templates/></span></code></em>
</xsl:template>

<xsl:template match="emph|emphasize">\fI<xsl:apply-templates/>\fP</xsl:template>

<xsl:template match="strong">\fB<xsl:apply-templates/>\fP</xsl:template>

<xsl:template match="important">
  <em><u><span class="important"><xsl:apply-templates/></span></u></em>
</xsl:template>

<xsl:template match="preformat">
  <br/>
    <pre><xsl:apply-templates/></pre>
  <br/>
</xsl:template>

<xsl:template match="reference-title"><xsl:text>
.SS </xsl:text></xsl:template>

<xsl:template match="reference-scheme"><xsl:apply-templates/></xsl:template>
<xsl:template match="reference-function">\fB\m[magenta]<xsl:apply-templates/>\m[]\fP</xsl:template>
<xsl:template match="reference-parameter">\fI\m[red]<xsl:apply-templates/>\m[]\fP</xsl:template>
<xsl:template match="reference-type">\fI\m[blue]<xsl:apply-templates/>\m[]\fP</xsl:template>
<xsl:template match="reference-returns">\h'-2m'\fI\m[green]<xsl:apply-templates/>\m[]\fP</xsl:template>
<!-- <xsl:template match="reference-blurb">\fI<xsl:apply-templates/>\fP</xsl:template> -->
<xsl:template match="reference-struct-name"> \fI\m[red]<xsl:apply-templates/>\m[]\fP</xsl:template>
<xsl:template match="reference-struct-open"> \fB{\fP</xsl:template>
<xsl:template match="reference-struct-close">\h'-4m'\fB};\fP</xsl:template>

<xsl:template match="keepspace">
  <span class="keepspace"><xsl:apply-templates/></span>
</xsl:template>

<xsl:template match="center">
  <div class="center" align="center"><xsl:apply-templates/></div>
</xsl:template>

<xsl:template match="linebreak">
  <br/>
</xsl:template>

<xsl:template match="image">
  <img>
    <xsl:if test="string-length(@alttext) > 0">
      <xsl:attribute name="alt">
	<xsl:value-of select="@alttext"/>
      </xsl:attribute>
    </xsl:if>
    <xsl:if test="string-length(@width) > 0">
      <xsl:attribute name="width">
	<xsl:value-of select="@width"/>
      </xsl:attribute>
    </xsl:if>
    <xsl:if test="string-length(@height) > 0">
      <xsl:attribute name="height">
	<xsl:value-of select="@height"/>
      </xsl:attribute>
    </xsl:if>
    <xsl:attribute name="src">
      <!-- <xsl:value-of select="concat($image_prefix, .)"/><xsl:text>.</xsl:text><xsl:value-of select="@extension"/> -->
    </xsl:attribute>
  </img>
</xsl:template>

<xsl:template match="para/indexterm">
  <a>
    <xsl:attribute name="name">
      <xsl:value-of select="@index"/><xsl:text>index-</xsl:text><xsl:number level="any"/>
    </xsl:attribute>
  </a>
</xsl:template>

<xsl:template match="printindex">
  <xsl:variable name="type" select="."/>
  <div class="index">
    <table summary="index" width="80%" border="0">
      <thead>
        <tr>
	  <td><strong>Name</strong></td>
	  <td><strong>Section</strong></td>
	</tr>
      </thead>
      <tbody>
	<xsl:for-each select="//para/indexterm[@index=$type]">
	  <xsl:sort/>
	  <tr>
	    <td width="40%">
	      <a>
		<xsl:attribute name="href">
		  <xsl:text>#</xsl:text><xsl:value-of select="$type"/><xsl:text>index-</xsl:text><xsl:number level="any"/>
		</xsl:attribute>
		<span class="index">
		  <xsl:apply-templates/>
		</span>
	      </a>
	    </td>
	    <td>
	      <xsl:value-of select="../../title"/>
	    </td>
	  </tr>
	</xsl:for-each>
      </tbody>
    </table>
  </div>
</xsl:template>

<xsl:template match="para/printplainindex">
  <xsl:variable name="type" select="."/>
  <xsl:for-each select="//para/indexterm[@index=$type]">
    <xsl:sort/>
      <xsl:apply-templates/><xsl:text>
.br
</xsl:text>
  </xsl:for-each>
</xsl:template>

<xsl:template match="para/news-date">
  <span class="news-date">
    <xsl:apply-templates/>
  </span>
  <br/>
</xsl:template>

<xsl:template match="para/news-title">
  <strong>
    <span class="news-title">
      <xsl:apply-templates/>
    </span>
  </strong>
  <br/>
</xsl:template>

</xsl:stylesheet>
