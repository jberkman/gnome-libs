#!/usr/bin/perl

require 5.004;
use Carp;
use strict;

my $input = join '', <>;

while ($input =~ m,/\*\*(.*?)\*+/,sg) {
  my $doc = $1;

  # Extract function name.

  $doc =~ s/^[\s\*]*//;
  next unless $doc =~ s/^(\w+)\b//;
  my $funcname = $1;

  $doc =~ s/^://; $doc =~ s/^\s*//;

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

  $retval =~ s,^\s*,,s; $retval =~ s,\s*$,,;
  $description =~ s,^\s*,,s; $description =~ s,\s*$,,s;

  print "FUNCTION: |$funcname|\n";
  print "RETVAL: |$retval|\n" unless $retval eq '';

  foreach (@paramdefs) {
    print "PARAM: |$_->[0]|$_->[1]|\n";
  }

  print "\n";

  print "DESCRIPTION: |$description|\n";
  
  print "\n\n";
}
