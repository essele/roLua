#!/usr/bin/perl -w


#
# Subroutine for pulling a list from a C source file
#
# Takes the filename, and a startline match expression
#
sub getlist {
    my $filename = shift @_;
    my $match = shift @_;

    open(FH, $filename) or die $!;
    while(<FH>) {
        if($_ =~ $match) {
            last;
        }
    }
    my $code = "(";
    while(<FH>) {
        last if (/};/);
        $code .= $_;
    }
    close(FH);
    $code .= ");";
    my @res = eval($code) or die $!;
    return @res;
}

sub getreg {
    my $filename = shift @_;
    my $match = shift @_;
    my %rc = ();

    open(FH, $filename) or die $!;
    while(<FH>) {
        last if($_ =~ $match);
    }
    while(<FH>) {
        last if (/};/);
        if ( $_ =~ /"([^"]+)",\s+(\w+)/ ) {
            my ($name,$func) = ($1, $2);
            next if $name eq "_VERSION";
            next if $func eq "NULL";
            $rc{$name} = $func;
        }
    }
    close(FH);
    return %rc;
}


#
# Pull the tokens from llex.c
#
@toks = getlist("src/llex.c", "luaX_tokens.*\\[\\] =");

#
# Pull the meta methods from ltm.c
#
@mm = getlist("src/ltm.c", "luaT_eventname.*\\[\\] =");

#
# Pull the items from baselib...
#
%baselib = getreg("src/lbaselib.c", "^const luaL_Reg base_funcs\\[\\] =");

#
# Now output the relevant code ... tokens need the token number
# as well, also create a mapping back to number for use later on.
#

# ------------------------------------------------------------------------------
# First write ro_main.h
# ------------------------------------------------------------------------------

open(FH_MAIN, ">ro_main.h") or die $!;
open(FH_BASELIB, ">ro_baselib.h") or die $!;

%map = {};
print FH_MAIN "//\n";
print FH_MAIN "// Autogenerated file, use: ./build_rostrings.pl\n";
print FH_MAIN "//\n";
print FH_MAIN "\n";

my $n = 0;

print FH_MAIN "//\n";
print FH_MAIN "// Tokens from llex.c\n";
print FH_MAIN "//\n";
foreach $i (@toks) {
    printf(FH_MAIN "static const roTString ros%03d = ROSTRING(\"%s\", %d);\n", $n, $i, $n+1 );
    $map{$i} = $n;
    $n++
}
print FH_MAIN "//\n";
print FH_MAIN "// Metamethods from ltm.c\n";
print FH_MAIN "//\n";
foreach $i (@mm) {
    printf(FH_MAIN "static const roTString ros%03d = ROSTRING(\"%s\", 0);\n", $n, $i );
    $map{$i} = $n;
    $n++
}

#
# The baselib functions need to be handled in a separate file, but need to be
# accessable from the main file so need to be extern's....
#

print FH_BASELIB "//\n";
print FH_BASELIB "// Autogenerated file, use: ./build_rostrings.pl\n";
print FH_BASELIB "//\n";
print FH_BASELIB "\n";
print FH_BASELIB "//\n";
print FH_BASELIB "// Functions from lbaselib.c\n";
print FH_BASELIB "//\n";
print FH_MAIN "\n";
print FH_MAIN "//\n";
print FH_MAIN "// Functions from lbaselib.c\n";
print FH_MAIN "//\n";
foreach $key (sort keys %baselib) {
    printf(FH_BASELIB "extern const roTString ros%03d;\n", $n);
    printf(FH_BASELIB "extern const roTString ros%03d = ROFUNC(\"%s\", %s);\n", $n, $key, $baselib{$key} );
    printf(FH_MAIN "extern const roTString ros%03d;\n", $n);
    # Store the number for the lookup table...
    $map{$key} = $n;
    $n++;
}

#
#
#


#
# Decided not to use buckets, as a quick-find algorithm will find the right string
# with 5 compares maximum and that's much simpler than anything else, we just need
# to ensure the list is sorted.
#
my @all = sort (@toks, @mm, keys %baselib);

#
# Now the list... (TODO: quick access)
#
print FH_MAIN "//\n";
print FH_MAIN "// The overall list of read only strings, sorted for quck-search\n";
print FH_MAIN "//\n";
print FH_MAIN "static const roTString *ro_tstrings[] = {\n";

my $minlen = 999;
my $maxlen = 0;
my $n = 0;      # for output styile
foreach $key (@all) {
    my $i = $map{$key};
    my $id = sprintf("%03d", $i);

    $minlen = length($key) if length($key) < $minlen;
    $maxlen = length($key) if length($key) > $maxlen;

    print(FH_MAIN "\t") if ($n % 8 == 0);
    printf(FH_MAIN "&ros%03d, ", $i);
    print(FH_MAIN "\n") if ($n % 8 == 7);   
    $n++;
}
print(FH_MAIN "\n") if ($n % 8 != 0);
print(FH_MAIN "};\n");
print(FH_MAIN "\n");
printf(FH_MAIN "#define MAX_ROSTRINGS (%d)\n", $n);
printf(FH_MAIN "#define MIN_ROSTRING_LEN (%d)\n", $minlen);
printf(FH_MAIN "#define MAX_ROSTRING_LEN (%d)\n", $maxlen);

print(FH_MAIN "\n");
print(FH_MAIN "//\n");
print(FH_MAIN "// Specific list for lbaselib functions\n");
print(FH_MAIN "//\n");
print FH_MAIN "static const roTString *ro_baselib[] = {\n";

$minlen = 999;
$maxlen = 0;
$n = 0;
foreach $key (sort keys %baselib) {
    my $i = $map{$key};
    my $id = sprintf("%03d", $i);

    $minlen = length($key) if length($key) < $minlen;
    $maxlen = length($key) if length($key) > $maxlen;

    print(FH_MAIN "\t") if ($n % 8 == 0);
    printf(FH_MAIN "&ros%03d, ", $i);
    print(FH_MAIN "\n") if ($n % 8 == 7);   
    $n++;
}
print(FH_MAIN "\n") if ($n % 8 != 0);
print(FH_MAIN "};\n");
print(FH_MAIN "\n");
printf(FH_MAIN "#define MAX_BASELIB (%d)\n", $n);
printf(FH_MAIN "#define MIN_BASELIB_LEN (%d)\n", $minlen);
printf(FH_MAIN "#define MAX_BASELIB_LEN (%d)\n", $maxlen);

exit(0);


