# Name:     merge10.pl
# Author:   Trevor Williams  (phase1geo@gmail.com)
# Date:     11/14/2008
# Purpose:  Verifies that each instance in the design can be merged in any order to make the same
#           base CDD file. 

require "../verilog/regress_subs.pl";

# Initialize the diagnostic environment
&initialize( "merge10", 1, @ARGV );

# Perform diagnostic running code here
&runScoreCommand( "-t main -vcd merge10.vcd -v merge10.v -o merge10.cdd" );

# Perform the file comparison checks
&checkTest( "merge10", 1, 1 );

exit 0;

sub doMerges {

  $bits = 3;
  for( $i=0; $i<=(8-$bits); $i++ ) {
    $allset |= (0x1 << $i);
  }
  print "bits: $bits, allset: $allset\n";

  $num = 0;

  for( $a=0; $a<8; $a++ ) {
    for( $b=0; $b<8; $b++ ) {
      if( $b!=$a ) {
        for( $c=0; $c<8; $c++ ) {
          if( ($c!=$a) && ($c!=$b) ) {
            for( $d=0; $d<8; $d++ ) {
              if( ($d!=$a) && ($d!=$b) && ($d!=$c) ) {
                for( $e=0; $e<8; $e++ ) {
                  if( ($e!=$a) && ($e!=$b) && ($e!=$c) && ($e!=$d) ) {
                    for( $f=0; $f<8; $f++ ) {
                      if( ($f!=$a) && ($f!=$b) && ($f!=$c) && ($f!=$d) && ($f!=$e) ) {
                        for( $g=0; $g<8; $g++ ) {
                          if( ($g!=$a) && ($g!=$b) && ($g!=$c) && ($g!=$d) && ($g!=$e) && ($g!=$f) ) {
                            for( $h=0; $h<8; $h++ ) {
                              if( ($h!=$a) && ($h!=$b) && ($h!=$c) && ($h!=$d) && ($h!=$e) && ($h!=$f) && ($h!=$g) ) {
                                $str[@str] = "$a$b$c$d$e$f$g$h";
                              }
                            }
                          }
                        }
                      }
                    }
                  }
                }
              }
            }
          }
        }
      }
    }
  }
  
  for( $k=0; $k<@str; $k++ ) {
    # print "k: $k, set: $set[$k]\n";
    if( $set[$k] != $allset ) {
      for( $j=0; $j<=(8-$bits); $j++ ) {
        if( (($set[$k] >> $j) & 0x1) == 0 ) {
          $sstr = substr( $str[$k], $j, $bits );
          for( $l=0; $l<@str; $l++ ) {
            if( $l != $k ) {
              for( $m=0; $m<=(8-$bits); $m++ ) {
                if( ((($set[$l] >> $m) & 0x1) == 0) && (substr( $str[$l], $m, $bits ) eq $sstr) ) {
                  $set[$l] |= (0x1 << $m);
                }
              }
            }
          }
        }
      }
    }
  }

  for( $i=0; $i<@str; $i++ ) {
    if( $set[$i] != $allset ) {
      $new_str[@new_str] = $str[$i];
      print "  $str[$i]\n";
    }
  }

  print "new_str num: " . @new_str . "\n";

}
