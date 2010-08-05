#!/usr/bin/awk -f
# Author: Tobi Vollebregt
# Purpose: convert a piece of changelog to BBcode
# The changelog must consist of headings (any line that ends with ":")
# and bullet lists (any non header line, must start with " - ") only.

# Headings
/^.*:$/ {
	if (expect_nonfirst_heading) {
		expect_nonfirst_heading = 0;
		print "[/list]\n";
	}
	print "[b]" $0 "[/b]";
	expect_first_bullet = 1;
}

# Bullets
/^ - (.*)$/ {
	gsub(/^ - /, "[*]");
	if (expect_first_bullet) {
		expect_first_bullet = 0;
		$0 = "[list]" $0;
	}
	print;
	expect_nonfirst_heading = 1;
}

END {
	if (expect_nonfirst_heading) {
		print "[/list]\n";
	}
}
