#!/usr/bin/perl -w

%LIB = ();
%STRINGS = ();
$LIBNAME = "";
$INDEX = 100;

#
# Process strings and tokens...
#
READ_STRINGS("src/llex.c", "luaX_tokens [] =", 1);
READ_STRINGS("src/ltm.c", "luaT_eventname[]");

#
# Math lib takes a bit of work...
#
READ_LIB("mathlib", "src/lmathlib.c", "mathlib[]");
REMOVE("random");
REMOVE("randomseed");
SET("huge", "int", "(lua_Number)HUGE_VAL");
SET("maxinteger", "int", "LUA_MAXINTEGER");
SET("mininteger", "int", "LUA_MININTEGER");
SET("pi", "num", "PI");
PROCESS_LIB();

#
# String lib
#
READ_LIB("strlib", "src/lstrlib.c", "strlib[]");
PROCESS_LIB();

#
# Table lib
#
READ_LIB("tablib", "src/ltablib.c", "tab_funcs[]");
PROCESS_LIB();

#
# Co-routines
#
READ_LIB("corolib", "src/lcorolib.c", "co_funcs[]");
PROCESS_LIB();

#
# Debug lib
#
READ_LIB("dblib", "src/ldblib.c", "dblib[]");
PROCESS_LIB();

#
# UTF-8 lib
#
READ_LIB("utf8lib", "src/lutf8lib.c", "funcs[]");
SET("charpattern", "string", "UTF8PATT");
PROCESS_LIB();

#
# Now build the base and add in the libs we actually want to use
#
READ_LIB("baselib", "src/lbaselib.c", "base_funcs[]");
SET("_VERSION", "string", "LUA_VERSION");
INCLUDE_LIB("math", "mathlib");
INCLUDE_LIB("string", "strlib");
INCLUDE_LIB("table", "tablib");
PROCESS_LIB();

#
# Now write the ro_main.h file, which is the tokens and strings and the
# associated list.
#
PROCESS_MAIN();
exit(0);

# ------------------------------------------------------------------------------
# Include a lib in the overall word table along with the relevant entry in
# baselib so it's globally accessible.
# ------------------------------------------------------------------------------
sub INCLUDE_LIB {
    my ($word, $libname) = @_;
    SET($word, "table", "ro_list_${libname}");

    foreach my $k (keys %STRINGS) {
        if ($STRINGS{$k}{"source"} eq $libname) {
            $STRINGS{$k}{"inc"} = 1;
        }
    }
}
# ------------------------------------------------------------------------------
# Add a word to the word list, but don't replace an existing one so we ensure
# the tokens stay in place even if there's a dup word in a library
# ------------------------------------------------------------------------------
sub _add_string {
    my ($string, $token, $index, $source) = @_;

    if (not exists $STRINGS{$string}) {
        $STRINGS{$string} = { "index"=>$index, "token"=>$token, "source"=>$source} 
    }
}
# ------------------------------------------------------------------------------
# Reads tokens from a list within a C source file, if $tok is defined then the
# output will include an incrementing token number.
# ------------------------------------------------------------------------------
sub READ_STRINGS {
    my $file = shift @_;
    my $match = shift @_;
    my $tok = shift @_;

    $tok = 0 if (not defined $tok);

    open(FH, $file) or die $!;
    while(<FH>) {
        last if($_ =~ /\Q$match\E/);
    }
    my $code = "(";
    while(<FH>) {
        last if (/};/);
        $code .= $_;
    }
    close(FH);
    $code .= ");";
    my @res = eval($code) or die $!;

    #
    # Now we can add each string to our strings list (this will uniq them)
    #
    foreach my $k (@res) {
        _add_string($k, $tok, $INDEX++, "main");
        $tok++ if($tok > 0);
    }
}

# ------------------------------------------------------------------------------
# Output a formatted list of things...
# ------------------------------------------------------------------------------
sub list_of {
    my $n = shift @_;
    my @list = @_;
    my $rc = "";
    my $i = 0;
    
    foreach my $item (@list) {
        $rc .= ", " if ($i != 0);              # comma after, if we have more
        $rc .= "\n\t\t" if ($i % $n == 0);      
        $rc .= "${item}";
        $i++;
    }
    $rc .= "\n" if ($i % $n == 0);
    return $rc;
}

# ------------------------------------------------------------------------------
# Update a specific entry in a library set..
# ------------------------------------------------------------------------------
sub SET {
    my $name = shift @_;
    my $type = shift @_;
    my $item = shift @_;
    
    if (exists $LIB{$name}) {
        $LIB{$name}{"type"} = $type;
        $LIB{$name}{"item"} = $item;
    } else {
        $LIB{$name} = { "type"=>$type, "item"=>$item, "index"=>$INDEX++ }
    }
}
sub REMOVE {
    my $name = shift @_;

    delete $LIB{$name};
}

# ------------------------------------------------------------------------------
# Reads the strings and functions from the lib source file and stores them in the
# global LIB array (which is zeroed at the start)
# ------------------------------------------------------------------------------
sub READ_LIB {
    my $name = shift @_;
    my $file = shift @_;
    my $match = shift @_;

    %LIB = ();
    $LIBNAME = $name;
    open(FH, $file) or die $!;
    while(<FH>) {
        last if ($_ =~ /\Q$match\E/);
    }
    while(<FH>) {
        last if /};/;
        if ( $_ =~ /"([^"]+)"\s*,\s+(\w+)/ ) {
            $LIB{$1} = { "type"=>"func", "item"=>$2, "index"=>$INDEX++ }
        }
    }
    close(FH);
}

# ------------------------------------------------------------------------------
# Process the include file for a given library, 
# global LIB array (which is zeroed at the start)
# ------------------------------------------------------------------------------
sub PROCESS_LIB {
    my $minlen = 9999;
    my $maxlen = 0;

    open(FH, ">src/ro_${LIBNAME}.h") or die $!;
    printf(FH "//\n// Automatically generated roLua file\n//\n");
    foreach my $k (sort keys %LIB) {
        $maxlen = length($k) if (length($k) > $maxlen);
        $minlen = length($k) if (length($k) < $minlen);

        my $type = $LIB{$k}{"type"};
        my $macro = "RO_UNKNOWN";

        $macro = "RO_DEF_FUNC" if ($type eq "func");
        $macro = "RO_DEF_INT" if ($type eq "int");
        $macro = "RO_DEF_NUM" if ($type eq "num");
        $macro = "RO_DEF_STRING" if ($type eq "string");
        $macro = "RO_DEF_TABLE" if ($type eq "table");
        printf(FH "${macro}(ros%03d, \"%s\", %s);\n", $LIB{$k}{"index"}, $k, $LIB{$k}{"item"});
        _add_string($k, 0, $LIB{$k}{"index"}, $LIBNAME);
    }
    printf(FH "//\n// List of items\n//\n");
    my @list = map { sprintf("&ros%03d", $LIB{$_}{"index"}) } sort keys %LIB; 
    printf(FH "RO_DEF_LIST(ro_list_${LIBNAME}, ");
    printf(FH list_of(8, @list));
    printf(FH ");\n");
    printf(FH "\n//\n// Metrics\n//\n");
    printf(FH "#define MIN_%s_LEN %d\n", uc(${LIBNAME}), $minlen);
    printf(FH "#define MAX_%s_LEN %d\n", uc(${LIBNAME}), $maxlen);
    close(FH);
}

# ------------------------------------------------------------------------------
# 
# ------------------------------------------------------------------------------
sub PROCESS_MAIN {
    my $minlen = 9999;
    my $maxlen = 0;

    open(FH, ">src/ro_main.h") or die $!;
    printf(FH "//\n// Automatically generated roLua file\n//\n");
    foreach my $k (sort keys %STRINGS) {
        $maxlen = length($k) if (length($k) > $maxlen);
        $minlen = length($k) if (length($k) < $minlen);

        my $tok = $STRINGS{$k}{"token"};
        my $index = $STRINGS{$k}{"index"};
        my $source = $STRINGS{$k}{"source"};

        next if $source ne "main";

        if ($tok) {
            printf(FH "const ro_TString ros%03d = RO_TOKEN(\"%s\", %s);\n", $index, $k, $tok);
        } else {
            printf(FH "const ro_TString ros%03d = RO_WORD(\"%s\");\n", $index, $k);
        }
    }
    printf(FH "//\n// List of items\n//\n");
    #
    # This list needs to be a "main" words and then any from included libraries
    #
    my @words = grep { $STRINGS{$_}{"source"} eq "main" or
                            defined $STRINGS{$_}{"inc"} } sort keys %STRINGS;
    my @list = map { sprintf("&ros%03d", $STRINGS{$_}{"index"}) } @words;
    printf(FH "RO_DEF_LIST(ro_list_tstrings, ");
    printf(FH list_of(8, @list));
    printf(FH ");\n");
    printf(FH "\n//\n// Metrics\n//\n");
    printf(FH "#define MIN_ROSTRING_LEN %d\n", $minlen);
    printf(FH "#define MAX_ROSTRING_LEN %d\n", $maxlen);
    printf(FH "\n//\n// Fast Lookup Ranges\n//\n");

    # Let's build a start/end mechanism for the first character is each word, it will
    # be a little inefficient for all the __xxx words, but is nice and simple and should
    # speed up detection of things that aren't in the list.
    my @ranges = ();
    for (my $i = 33; $i <= 127; $i++) { $ranges[$i] = { "start"=>-1, "end"=>-1 }; }
    for (my $i=0; $i <= $#words; $i++) {
        my $ch = ord(substr($words[$i], 0, 1));
        die "INVALID CHAR" if ($ch < 33 or $ch > 127);
        $ranges[$ch]{"end"} = $i;
    }
    for (my $i=$#words; $i >= 0; $i--) {
        my $ch = ord(substr($words[$i], 0, 1));
        die "INVALID CHAR" if ($ch < 33 or $ch > 127);
        $ranges[$ch]{"start"} = $i;
    }

    my @rr = ();
    for (my $i = 33; $i <= 127; $i++) {
         push (@rr, sprintf("{%d,%d}",
                            $ranges[$i]{"start"}, $ranges[$i]{"end"}));
    }
    printf(FH "const ro_range ro_range_lookup[] = {");
    printf(FH list_of(8, @rr));
    printf(FH "};\n\n");
    close(FH);
}
