#!/usr/bin/perl

while($aline = <>) {

  # ignore some stuff
  if($aline =~ /^\#/
    || $aline =~ /GNOME_DECLS/si) {
    next;
  }

  chomp($aline);

  if(length($aline) < 2) { next; }

#  if($aline =~ /\)$/si) {
#    next;
#  }
  $theline .= $aline;

  if($aline =~ /;$/si) {
    $theline =~ s/\s+/ /sig;
    if($aline =~ /\);$/si && $theline !~ /typedef/si) {
      # wherein we parse the actual function prototype
      $theline =~ /([a-z_0-9\s]+)(\s+\**|\**\s+)([a-z_0-9]+)\s*\((.*)\);$/si;
      $retval = $1.$2; $funcname = $3; $paramlist = $4;
      $funcname =~ s/^_//sig;
      $paramlist =~ s/^\s+//sig;
      $paramlist =~ s/\s+$//sig;
      @paraml = (); @paramtypes = ();
      if($paramlist ne "void" && length($paramlist) > 1) { 
	@paramtmp = split(/\s*,\s*/, $paramlist);
	foreach $aparam(@paramtmp) {
	  $aparam =~ /^([a-z_0-9\s]+)(\s+\**|\**\s+)(\s*const\s*)?([a-z_0-9]+)(\[\])?$/si;
	  $ptype = $1.$2.$3.$5; $pname = $4;
	  $ptype =~ s/^\s+//sig;
	  $ptype =~ s/\s+$//sig;
	  $pname =~ s/^\s+//sig;
	  $pname =~ s/\s+$//sig;
	  if(length($pname) > 0 && length($ptype) > 1) {
	    push(@paraml, $pname);
	    push(@paramtypes, $ptype);
	  }
	}
      }
      $secname = $funcname; $secname =~ s/_/-/sig;
      # wherein we print out the SGML code for this funcdef
      print <<EOF;
<sect2 id="$secname"><title>$funcname - </title>
<funcsynopsis><funcdef>$retval <function>$funcname</function></funcdef>
EOF
  for($i = 0; $i < scalar(@paraml); $i++) {
    printf "<paramdef>%s <parameter>%s</parameter></paramdef>\n", $paramtypes[$i],
    $paraml[$i];
  }
      print <<EOF;
</funcsynopsis>
<sect3><title>Description</title>
<para>
</para>
</sect3>
<sect3><title>Usage</title>
<programlisting>
$theline
</programlisting>
</sect3>
EOF

if(scalar(@paraml) > 0) {
      print <<EOF;
<sect3><title>Parameters</title>
<itemizedlist>
EOF
  for($i = 0; $i < scalar(@paraml); $i++) {
    printf "<listitem>\n<para>%s <parameter>%s</parameter></para>\n<para>\n</para></listitem>\n", $paramtypes[$i],
    $paraml[$i];
  }
  print <<EOF;
</itemizedlist>
</sect3>
EOF
}
print "</sect2>\n";
    } 

    $theline = "";
  }
}
