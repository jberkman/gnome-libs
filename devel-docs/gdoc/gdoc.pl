#!/usr/bin/perl

$| = 1;

$file = $ARGV[0];

eval(`gdoc -v $file`);
$file =~ tr/_/-/;
$file =~ s#^.*/([^/]*).h$#$1#;

%fc = split_comm($FILE_COMMENT);

print "<sect1 id=\"$file\">\n";
print "  <title>$file" . 
	($fc{'brief'} ? " - $fc{'brief'}" : "") .
	"</title>\n";

printcomment(2, %fc);

foreach $d (@DECL) {
	%comm = split_comm( $d->{'comm'} );
	$comm{'Returns'} = $d->{'ret'} if $d->{'ret'};

	$id = $d->{name};
	$id =~ tr/_/-/;

	print "<sect2 id=\"$id\">\n";
	print "  <title>$d->{name}" . 
		($comm{'brief'} ? " - $comm{'brief'}" : "") .
		"</title>\n";

	if( $d->{type} == 0 || $d->{type} == 1 ) {
		print "<synopsis>$d->{spec} $d->{text}</synopsis>\n";
		printcomment(3, %comm);
		}
	elsif ( $d->{type} == 2 ) {
		print_prototipe ($d);
		printcomment(3, %comm);
		printparams(3, $d->{'param'}) if @{$d->{'param'}};
		}
	else {
		die "Unknown type of declaration: $d->{type}\n";
		}

	print "</sect2>\n";
	}

print "</sect1>\n";

sub
split_comm 
{
  my %sect2canon = (
  	'DESC' => 'Description',
  	'DESCRIPTION' => 'Description',
  	'USAGE' => 'Usage',
  	'USG' => 'Usage',
  	'AUT' => 'Author(s)',
	'AUTH' => 'Author(s)',
	'AUTHOR' => 'Author(s)',
	'AUTHORS' => 'Author(s)',
	);
 
  my @c = split( '\n', shift);
  my %c;
  my $s = 'Description';
  my $l;

  $c{'brief'} = shift @c
  	unless ( ($c[0] =~ /^[A-Z]+:$/) || # not a section definition
  		 $c[1] ); # one line comment

do {
	shift @c while( @c && !$c[0]);

	while ( defined ( $l = shift @c ) ) {
		if ( $l =~ /^([A-Z]+):$/ ) {
			$s = $sect2canon{$1} ||
			     die "Error: unknown section $1\n";
			last;
			}
		$c{$s} .= "$l\n";
		}
  	} while(@c);

  if(!$c{'Description'}) {
	$c{'Description'} = $c{'brief'} || "Not descripted"; }
 
  return %c;
}

sub 
printcomment
{
my $sect = shift;
my %fc = @_;
my $s;

foreach $s (sort keys %fc) {
	next if $s eq 'brief';

	print "<sect$sect><title>$s</title>\n";
	if ( $s eq 'Usage' ) {
		print "<programlisting>\n";
		open IND, '|indent - | sed s/\</\\\\\\&lt\;/g\;'.
				          's/\>/\\\\\\&gt\;/g';
		print IND $fc{$s};
		close IND;
		print "</programlisting>\n";
		} else {
		$fc{$s} =~ s/</&lt;/g;
		$fc{$s} =~ s/>/&gt;/g;
		$fc{$s} =~ s/\n\n+(?=.)/<\/para>\n<para>/g;
		print "<para>\n$fc{$s}</para>\n";
		}
	print "</sect$sect>\n\n";
	}
}

sub
print_prototipe {

$d = shift;
	
$d->{text} =~ s/%s\(\)$//;
	
print "<funcsynopsis>";
#print "<funcprototype>\n";
print "<funcdef> $d->{spec} $d->{text}<function>$d->{name}</function>\n";
print "</funcdef>\n";

my $p;
foreach $p ( @{$d->{param}} ) {
	my ($stars, $name) = ($p->{name} =~ /^(\**)(.*)$/);
	print "<paramdef>\n";
	print "$p->{type} $stars<parameter>$name</parameter>\n";
	print "</paramdef>\n";
	}

#print "</funcprototype>";
print "</funcsynopsis>\n";
}

sub
printparams
{
my $sect = shift;
my $param = shift;

print "<sect3><title>Parameters</title>\n";
print "<itemizedlist>\n";

my $p;
foreach $p (@$param) {
	my ($stars, $name) = ($p->{name} =~ /^(\**)(.*)$/);
	print "<listitem>\n";
	print "<para>$p->{type} $stars<parameter>$name</parameter></para>\n";
	$p->{comm} = "Not Descripted." unless $p->{comm};
	print "<para>$p->{comm}</para>\n";
	print "</listitem>\n";
	}

print "</itemizedlist>\n";
print "</sect3>\n";
}
