<?xml version="1.0"?>
<xsl:stylesheet xmlns:xsl="http://www.w3.org/1999/XSL/Transform" version="1.0">
<xsl:output method="text" indent="no" charset="UTF-8"/>
<xsl:strip-space elements="*"/>
<xsl:preserve-space elements="keepspace preformat programlisting reference-scheme"/>

<xsl:param name="revision"/>
<xsl:param name="man_section"/>

<xsl:template match="texinfo">.\" t<xsl:call-template name="document-font"/>
.\" Understrike
.de us
\\$1\l'|0\(ul'
..
.\" Paragraph
.de pg
.ft R
.ps 10
.vs 12p
.in 0
.sp 0.4
.ne 1+\\n(.Vu
.ti 0.2i
..
.nh
.fp 5 C
.fp 6 CI
.TH "<xsl:value-of select="settitle"/>" "<xsl:value-of select="$man_section"/>" "<xsl:value-of select="$revision"/>" "<xsl:value-of select="para/document-package"/>" "<xsl:value-of select="para/document-package"/>"
<xsl:call-template name="title_page"/><xsl:apply-templates/>
</xsl:template>

<!-- <xsl:template name="man_header"><xsl:if test="not($man_section='')"><xsl:text>.TH "</xsl:text><xsl:value-of select="settitle"/>" "<xsl:value-of select="$man_section"/>" "<xsl:value-of select="$revision"/>" "<xsl:value-of select="para/document-package"/>" "<xsl:value-of select="para/document-package"/>" -->
<!-- </xsl:if></xsl:template> -->

<!-- useless tags -->
<xsl:template match="setfilename|settitle|document-title|document-author|document-package|document-font|itemfunction|columnfraction"/>

<xsl:template name="document-font">
  <xsl:variable name="font" select="string(/texinfo/para/document-font)"/>
  <xsl:choose>
    <xsl:when test="$font=''"/>
    <xsl:when test="$font='tech' or $font='techstyle' or $font='sans' or $font='sans-serif'"><xsl:text>
.fam H</xsl:text></xsl:when>
    <xsl:when test="$font='story' or $font='storystyle' or $font='serif'"><xsl:text>
.fam T</xsl:text></xsl:when>
    <xsl:otherwise>
      <xsl:message>XSL-WARNING: omitting unknown font style '<xsl:value-of select="$font"/>'</xsl:message>
    </xsl:otherwise>
  </xsl:choose>
</xsl:template>

<xsl:template name="title_page">
  <xsl:if test="string-length(/texinfo/para/document-title) > 0 or count(/texinfo/para/document-author) > 0">
    <xsl:if test="string-length(/texinfo/para/document-title) > 0"><xsl:text>.ce 2
\s+4\fB</xsl:text><xsl:value-of select="/texinfo/para/document-title"/><xsl:text>\fP\s0
.br
</xsl:text></xsl:if>
    <xsl:if test="count(/texinfo/para/document-author) > 0">
      <xsl:choose>
	<xsl:when test="count(/texinfo/para/document-author) > 1">
	  <xsl:for-each select="/texinfo/para/document-author">
	    <xsl:if test="position() > 1 and not(position()=last())">
	      <xsl:text>, </xsl:text>
	    </xsl:if>
	    <xsl:if test="position() mod 4 = 0"><xsl:text>
.br
.ce
</xsl:text></xsl:if>
	    <xsl:if test="position() = last()">
	      <xsl:text> and </xsl:text>
	    </xsl:if>
	    <xsl:apply-templates/>
	  </xsl:for-each>
	</xsl:when>
	<xsl:otherwise>
	  <xsl:for-each select="/texinfo/para/document-author">
	    <xsl:apply-templates/>
	  </xsl:for-each>
	</xsl:otherwise>
      </xsl:choose>
    </xsl:if><xsl:text>
</xsl:text>
  </xsl:if>
</xsl:template>

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
<xsl:template match="para/table-of-contents">
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
.in +2
</xsl:text>
    <xsl:for-each select="./section">
      <xsl:call-template name="toc_section"/>
    </xsl:for-each>
<xsl:text>
.in -2
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
.in +2
</xsl:text>
    <xsl:for-each select="./subsection">
      <xsl:call-template name="toc_subsection"/>
    </xsl:for-each>
<xsl:text>
.in -2
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
.in +2
</xsl:text>
    <xsl:for-each select="./subsubsection">
      <xsl:call-template name="toc_subsubsection"/>
    </xsl:for-each>
<xsl:text>
.in -2
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
.in +2
</xsl:text>
    <xsl:for-each select="./appendixsec">
      <xsl:call-template name="toc_appendixsec"/>
    </xsl:for-each>
<xsl:text>
.in -2
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
.in +2
</xsl:text>
    <xsl:for-each select="./appendixsubsec">
      <xsl:call-template name="toc_appendixsubsec"/>
    </xsl:for-each>
<xsl:text>
.in -2
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
.in +2
</xsl:text>
    <xsl:for-each select="./appendixsubsubsec">
      <xsl:call-template name="toc_appendixsubsubsec"/>
    </xsl:for-each>
<xsl:text>
.in -2
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
.in +2
</xsl:text>
    <xsl:for-each select="./unnumberedsec">
      <xsl:call-template name="toc_unnumberedsec"/>
    </xsl:for-each>
<xsl:text>
.in -2
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
.in +2
</xsl:text>
    <xsl:for-each select="./unnumberedsubsec">
      <xsl:call-template name="toc_unnumberedsubsec"/>
    </xsl:for-each>
<xsl:text>
.in -2
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
.in +2
</xsl:text>
    <xsl:for-each select="./unnumberedsubsubsec">
      <xsl:call-template name="toc_unnumberedsubsubsec"/>
    </xsl:for-each>
<xsl:text>
.in -2
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

<xsl:template match="enumerate|itemize"><xsl:text>.br
.na
.in +2
</xsl:text><xsl:apply-templates/><xsl:text>.in -2

.ad
</xsl:text></xsl:template>

<xsl:template match="itemize/item"><xsl:text>\(bu </xsl:text><xsl:apply-templates/><xsl:text>.br
</xsl:text></xsl:template>

<xsl:template match="enumerate/item"><xsl:number format="1."/> <xsl:apply-templates/><xsl:text>.br
</xsl:text></xsl:template>

<xsl:template match="multitable">.TS
nokeep;
l l l.
<xsl:apply-templates/><xsl:text>
.TE

</xsl:text>
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

<xsl:template match="para"><xsl:apply-templates/><xsl:text>
</xsl:text></xsl:template>

<xsl:template match="para-disabled">
  <xsl:choose>
    <!-- If this para is the parent of a revision or toc or some similar tag, then we omit the .PP -->
    <xsl:when test="(count(./revision) + count(./table-of-contents) + count(./reference-title) + count(./reference-struct-open) + count(./reference-struct-close)) > 0"><xsl:apply-templates/><xsl:text>
</xsl:text></xsl:when>
    <!-- If this is an item, then it's not a real paragraph -->
    <xsl:when test="local-name(..)='item'"><xsl:apply-templates/><xsl:text>
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
  <!-- protocol for this link type -->
  <xsl:variable name="protocol" select="substring-before(urefurl, '://')"/>
  <xsl:if test="$protocol=''">
    <xsl:message terminate="yes">XSL-ERROR: unset protocol for <xsl:value-of select="urefurl"/></xsl:message>
  </xsl:if>

  <!-- actual link -->
  <xsl:variable name="url" select="substring-after(urefurl, '://')"/>

  <!-- feedback -->
  <!-- <xsl:message>DEBUG: protocol is <xsl:value-of select="$protocol"/> for <xsl:value-of select="urefurl"/></xsl:message> -->

  <xsl:choose>
    <!-- PROTOCOL: HTTP FTP -->
    <xsl:when test="$protocol='http' or $protocol='ftp'">
      <xsl:choose>
	<xsl:when test="count(child::urefreplacement)"><xsl:apply-templates select="urefreplacement"/></xsl:when>
	<xsl:when test="count(child::urefdesc)"><xsl:apply-templates select="urefdesc"/> (\fI\f6<xsl:value-of select="concat($protocol, '://', $url)"/>\fP)</xsl:when>
	<xsl:otherwise>\fI\f6<xsl:value-of select="concat($protocol, '://', $url)"/>\fP</xsl:otherwise>
      </xsl:choose>
    </xsl:when>
    <!-- PROTOCOL: FILE MAILTO -->
    <xsl:when test="$protocol='file' or $protocol='mailto'">
      <xsl:choose>
	<xsl:when test="count(child::urefreplacement)"><xsl:apply-templates select="urefreplacement"/></xsl:when>
	<xsl:when test="count(child::urefdesc)"><xsl:apply-templates select="urefdesc"/> (<xsl:value-of select="$url"/>)</xsl:when>
	<xsl:otherwise><xsl:value-of select="$url"/></xsl:otherwise>
      </xsl:choose>
    </xsl:when>
    <!-- PROTOCOL: NEWS -->
    <xsl:when test="$protocol='news'">
      <xsl:choose>
	<xsl:when test="count(child::urefreplacement)"><xsl:apply-templates select="urefreplacement"/></xsl:when>
	<xsl:when test="count(child::urefdesc)"><xsl:apply-templates select="urefdesc"/> (\fI\f6<xsl:value-of select="concat($protocol, ':', $url)"/>\fP)</xsl:when>
	<xsl:otherwise>\fI\f6<xsl:value-of select="concat($protocol, ':', $url)"/>\fP</xsl:otherwise>
      </xsl:choose>
    </xsl:when>
    <!-- PROTOCOL: System and BEAST Man Pages -->
    <xsl:when test="$protocol='man' or $protocol='beast-man'">
      <!-- Get the section the man page belongs to -->
      <xsl:variable name="section">
	<xsl:choose>
	  <xsl:when test="substring-before($url, '/') = ''">
	    <xsl:message>XSL-WARNING: unset man section in <xsl:value-of select="urefurl"/>, using default (1)</xsl:message>
	    <xsl:value-of select="1"/>
	  </xsl:when>
	  <xsl:otherwise>
	    <xsl:value-of select="substring-before($url, '/')"/>
	  </xsl:otherwise>
	</xsl:choose>
      </xsl:variable>
      <!-- Name of the page -->
      <xsl:variable name="page">
	<xsl:choose>
	  <xsl:when test="substring-after($url, '/') = ''">
	    <xsl:value-of select="$url"/>
	  </xsl:when>
	  <xsl:otherwise>
	    <xsl:value-of select="substring-after($url, '/')"/>
	  </xsl:otherwise>
	</xsl:choose>
      </xsl:variable>
      <!-- Print it -->
      <xsl:choose>
	<xsl:when test="count(child::urefreplacement)"><xsl:apply-templates select="urefreplacement"/></xsl:when>
	<xsl:when test="count(child::urefdesc)"><xsl:apply-templates select="urefdesc"/> (<xsl:value-of select="concat($page, '(', $section, ')')"/>)</xsl:when>
	<xsl:otherwise><xsl:value-of select="concat($page, '(', $section, ')')"/></xsl:otherwise>
      </xsl:choose>
    </xsl:when>
    <!-- PROTOCOL: Beast Document -->
    <xsl:when test="$protocol='beast-doc'">
      <!-- Get the file name and append the target specific extension (html) -->
      <xsl:variable name="filename">
	<xsl:choose>
	  <xsl:when test="substring-before($url, '#') = ''">
	    <xsl:value-of select="concat($url, '(1)')"/>
	  </xsl:when>
	  <xsl:otherwise>
	    <xsl:value-of select="concat(substring-before($url, '#'), '(1)')"/>
	  </xsl:otherwise>
	</xsl:choose>
      </xsl:variable>
      <!-- Get the anchor -->
      <xsl:variable name="anchor">
	<xsl:choose>
	  <xsl:when test="substring-after($url, '#') = ''"/>
	  <xsl:otherwise>
	    <xsl:value-of select="concat(', ', substring-after($url, '#'))"/>
	  </xsl:otherwise>
	</xsl:choose>
      </xsl:variable>
      <!-- Print the link -->
      <xsl:choose>
	<xsl:when test="count(child::urefreplacement)"><xsl:apply-templates select="urefreplacement"/></xsl:when>
	<xsl:when test="count(child::urefdesc)"><xsl:apply-templates select="urefdesc"/> (<xsl:value-of select="concat($filename, $anchor)"/>)</xsl:when>
	<xsl:otherwise><xsl:value-of select="concat($filename, $anchor)"/></xsl:otherwise>
      </xsl:choose>
    </xsl:when>
    <!-- Unknown Protocol -->
    <xsl:otherwise>
      <xsl:message>XSL-WARNING: unknown protocol '<xsl:value-of select="$protocol"/>' in <xsl:value-of select="urefurl"/>, using as-is</xsl:message>
      <xsl:choose>
	<xsl:when test="count(child::urefreplacement)"><xsl:apply-templates select="urefreplacement"/></xsl:when>
	<xsl:when test="count(child::urefdesc)"><xsl:apply-templates select="urefdesc"/> (\fI\f6<xsl:value-of select="urefurl"/>\fP)</xsl:when>
	<xsl:otherwise>\fI\f6<xsl:value-of select="urefurl"/>\fP</xsl:otherwise>
      </xsl:choose>
    </xsl:otherwise>
  </xsl:choose>
</xsl:template>

<xsl:template match="code">\f5<xsl:apply-templates/>\f1</xsl:template>

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

<xsl:template match="reference-scheme"><xsl:apply-templates/><xsl:text>
.ti -4
</xsl:text></xsl:template>
<xsl:template match="reference-function">\fB\m[magenta]<xsl:apply-templates/>\m[]\fP</xsl:template>
<xsl:template match="reference-parameter">\fI\m[red]<xsl:apply-templates/>\m[]\fP</xsl:template>
<xsl:template match="reference-type">\fI\m[blue]<xsl:apply-templates/>\m[]\fP</xsl:template>
<xsl:template match="reference-returns">\h'-2m'\fI\m[green]<xsl:apply-templates/>\m[]\fP</xsl:template>
<!-- <xsl:template match="reference-blurb">\fI<xsl:apply-templates/>\fP</xsl:template> -->
<xsl:template match="reference-struct-name"> \fI\m[red]<xsl:apply-templates/>\m[]\fP</xsl:template>
<xsl:template match="reference-struct-open"> \fB{\fP</xsl:template>
<xsl:template match="reference-struct-close">\h'-4m'\fB};\fP<xsl:text>
</xsl:text></xsl:template>

<xsl:template match="keepspace">
.nf
.na
<xsl:apply-templates/>
.fi
.ad
</xsl:template>

<xsl:template match="center">.ce
<xsl:apply-templates/></xsl:template>

<xsl:template match="linebreak"><xsl:text>
</xsl:text></xsl:template>

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
<!-- vim: set fdm=marker: -->
