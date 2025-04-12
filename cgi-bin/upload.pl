#!/usr/bin/perl
use strict;
use warnings;
use CGI;
use File::Path qw(make_path);
use Cwd 'cwd';

my $q = CGI->new;
print $q->header;

my $filename = $q->param("file");
my $fh       = $q->upload("file");

unless ($filename && $fh) {
    print "<p>Error: No file uploaded.</p>";
    exit;
}

# Sanitize filename
$filename =~ s/.*[\/\\](.*)/$1/;

# Upload directory (relative to DOCUMENT_ROOT or current dir)
my $upload_dir = ($ENV{'DOCUMENT_ROOT'} // cwd()) . "/uploads";
make_path($upload_dir) unless -d $upload_dir;

# Save file
open(my $out, '>', "$upload_dir/$filename") or die "Can't save file: $!";
binmode $out;
while (<$fh>) {
    print $out $_;
}
close $out;

# Output link
print "<p>File uploaded successfully.</p>";
