#!/usr/bin/perl
# by Elliot Lee
# Scans a file of C source code for places where the H_() macro is used to set the widget name
# These widget names are used to build the context-sensitive help mapping tables

foreach $afile(@ARGV) {
  open(FH, $afile) || next;
  $curpos = 0;
  $eating = 0;
  
  $wholefile = join("", <FH>);
  
  while(($curpos = index($wholefile, "H_", $curpos)) >= 0) {

    $curpos += 2;

    if(substr($wholefile, $curpos - 3, 1) !~ /^[\s,;]$/) {
      next;
    }

    $eating = 1;

    while($eating) {
      $curchar = substr($wholefile, $curpos, 1);
      if($curchar eq " "
	 || $curchar eq "\r"
	 || $curchar eq "\n"
	 || $curchar eq "\t") {
	# Do nothing
      } elsif($curchar eq "(") {
	last; # OK it's bona-fide
      } else {
	$eating = 0;
      }
      $curpos++;
    }

    $preserveme = 0;
    $parencount = 0;
    $quotecount = 0;
    $escapecount = 0;
    $stringtok = "";
    
    while($eating) {
#      printf "Eating %s at %d\n", substr($wholefile, $curpos, 10), $curpos;

      $curchar = substr($wholefile, $curpos, 1);
      if($curchar eq "") {
	$eating = 0;
      } elsif($quotecount) {
	if(! $escapecount){
	  if($curchar eq "\"") {
	    $quotecount--;

	    if($preserveme) {
	      $eating = 0;
	      $preserveme = 0;
	    }
	  } elsif($curchar eq "\\") {
	    $escapecount++;
	  }
	} else {
	  $escapecount = 0;
	}

	if($quotecount && !$escapecount) {
	  $stringtok .= $curchar;
	}
      } elsif($curchar eq "\"") {
	$quotecount++;
	$stringtok = "";
      } elsif($curchar eq "(") {
	$parencount++;
      } elsif($curchar eq ")") {
	$parencount--;
	if($parencount < 0)
	  {
	    $eating = 0;
	  }
      } elsif($curchar eq "," && $parencount == 1) {
	$preserveme = 1;
      }
      # else ignore it

      $curpos++;
    }

    if(! $stringtok) {
      next;
    }

    if($items{$stringtok}) {
      die("Widget ID $stringtok already used!");
    }

    $items{$stringtok} = 1;
  }
  
  close(FH);
}

foreach $akey(sort { $a cmp $b } keys %items) {
  printf "%s=\n", $akey;
}
