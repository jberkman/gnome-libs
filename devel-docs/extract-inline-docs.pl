#!/usr/bin/perl

require 5.004;
use Carp;
use strict;

my $input = join '', <>;

while ($input =~ m,/\*\*(.*?)\*+/\s*(.*?)\{,sg) {
  my ($doc, $theline) = ($1, $2);

  # Extract function name.

  $doc =~ s/^[\s\*]*//;
  next unless $doc =~ s/^(\w+)\b//;
  my $funcname = $1;

  $doc =~ s/^://;

  $doc =~ s/^[^\n]*\n//;
  my $funcdescript = $&;

  $doc =~ s/^\s*//;

  # Split remaining text into lines.

  my @lines = ();
  while ($doc =~ m,^\s*\*(.*)$,mg) {
    my $line = $&; $line =~ s,^\s*\*\s*,,;
    push @lines, $line;
  }

  # Extract parameter description.

  my @params = ();

  my $curparam = '';
  while (defined (my $line = shift @lines)) {
    last if $line =~ /^\s*$/;

    unless ($line =~ /^@/) {
      $curparam .= "\n".$line;
      next;
    }

    push @params, $curparam unless $curparam =~ /^\s*$/s;
    $curparam = $line;
  }

  push @params, $curparam unless $curparam =~ /^\s*$/s;

  # Parse parameter descriptions.

  my @paramdefs = ();

  foreach (@params) {
    if (/^@(\w+\b)\s*$/) {
      push @paramdefs, [$1,''];
      next;
    }

    unless (/^@(\w+\b.*):\s*(.*)$/s) {
      warn "Invalid parameter description '$_'";
      next;
    }

    push @paramdefs, [$1,$2];
  }

  # Split remaining text into blocks separated by blank lines.

  my @blocks = ();

  my $curblock = '';
  while (defined (my $line = shift @lines)) {
    unless ($line =~ /^\s*$/) {
      $curblock .= "\n".$line;
      next;
    }

    push @blocks, $curblock unless $curblock =~ /^\s*$/s;
    $curblock = '';
  }

  push @blocks, $curblock unless $curblock =~ /^\s*$/s;

  # There may be blank lines inside a block, so join them again ...

  my @realblocks = ();

  $curblock = '';
  foreach (@blocks) {
    my $block = $_; $block =~ s/^\s*//;

    unless ($block =~ /^(\w+):/) {
      $curblock .= "\n".$block;
      next;
    }

    push @realblocks, $curblock unless $curblock =~ /^\s*$/s;
    $curblock = $block;
  }

  push @realblocks, $curblock unless $curblock =~ /^\s*$/s;

  # Ok, done. Now read the blocks.

  my $retval = '';
  my $description = '';

  foreach (@realblocks) {
    my $block = $_; $block =~ s,^\s*,,s;

    unless ($block =~ s/^(\w+)://) {
      $description .= "\n".$block;
      next;
    }

    my $keyword = $1; $block =~ s,^\s*,,s;

    if ($keyword =~ /^Return(s| value)$/i) {
      $retval .= "\n".$block;
      next;
    } elsif ($keyword =~ /^Description$/i) {
      $description .= "\n".$block;
      next;
    } else {
      warn "Unknown keyword '$keyword'";
      $description .= "\n".$block;
      next;
    }
  }

  # Parse function prototype

  $theline =~ s,^\s*,,s; $theline =~ s,\s*$,,s;

  {
    my $tmp = $theline; $tmp =~ s,^[^\(]*\n,,s;
    my $start = $&;

    my $length = index $tmp, '(';
    my $space = ' ' x ($length+1);

    $tmp =~ s,\n\s*,\1\n$space,sg;
    $tmp =~ s,\s*$,,s;

    $theline = $start.$tmp;
  }

  unless ($theline =~ /^(.*?)\s*(\w+)\s*\((.*?)\)$/s) {
    warn "Illegal function prototype '$theline'";
  }

  my $rettype = $1;
  unless ($funcname eq $2) {
    warn "Function called `$funcname' in documentation, but `$2' in declaration";
  }

  my (@dparams, %dparams);
  foreach (split /,/, $3) {
    my $param = $_;

    $param =~ s,^\s*,,s;
    $param =~ s,\s*$,,s;

    unless ($param =~ /^(.*?[\s\*])\s*(\w+.*)$/s) {
      warn "Illegal function parameter `$param'";
    }

    my ($type, $name, $full) = ($1, $2, $2);
    $full =~ s/^(\w+).*$/\1/;

    push @dparams, [$type, $full];
    $dparams{$name} = $type;
  }

  # Create output

  $retval =~ s,^\s*,,s; $retval =~ s,\s*$,,;
  $description =~ s,^\s*,,s; $description =~ s,\s*$,,s;
  $funcdescript =~ s,^\s*,,s; $funcdescript =~ s,\s*$,,s;

  my $secname = $funcname; $secname =~ tr/_/-/;

  $funcdescript = ' - '.$funcdescript unless
    $funcdescript eq '';

  # wherein we print out the SGML code for this funcdef
  print <<EOF;
<sect2 id="$secname"><title><function>$funcname</function>$funcdescript</title>
<funcsynopsis>
<funcdef>$rettype
<function>$funcname</function>
</funcdef>
EOF
  
  foreach (@dparams) {
    my ($type, $name) = ($_->[0], $_->[1]);
    print qq[<paramdef>$type <parameter>$name</parameter></paramdef>\n];
  }

  print qq[</funcsynopsis>\n];

  print <<EOF;
 <sect3><title>Usage</title>
<programlisting>
$theline;
</programlisting>
</sect3>
EOF

  unless ($retval eq '') {
    print <<EOF;
<sect3><title>Return Value</title>
<para>
$retval
</para>
</sect3>
EOF
}

  unless ($description eq '') {
    print <<EOF;
<sect3><title>Description</title>
<para>
$description
</para>
</sect3>
EOF
}

  my @realparamdefs;

  foreach (@paramdefs) {
    next if $_->[1] eq '';

    push @realparamdefs, $_;
  }

  if(scalar(@realparamdefs) > 0) {
    print <<EOF;
<sect3><title>Parameters</title>
<variablelist>
EOF
    foreach (@realparamdefs) {
      my ($name, $descr) = ($_->[0], $_->[1]);
      my $type = $dparams{$name};

      print <<EOF;
<varlistentry>
<term>
$type <parameter>$name</parameter>
</term>

<listitem>
<para>
$descr
</para>
</listitem>
</varlistentry>
EOF
}
    print <<EOF;
</variablelist>
</sect3>
EOF
  }

  print <<EOF;
</sect2>
EOF
    

#  print "FUNCTION: |$funcname|\n";
#  print "RETVAL: |$retval|\n" unless $retval eq '';

  foreach (@paramdefs) {
#    print "PARAM: |$_->[0]|$_->[1]|\n";
  }

#  print "\n";

#  print "DESCRIPTION: |$description|\n";
  
  print "\n\n";
}
