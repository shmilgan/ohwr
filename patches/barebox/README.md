Generation
============

These patches are generated using the following git:

	<https://github.com/neub/wrs-sw-barebox/tree/patcheable-wrs>
	
and by executing:

	git format-patch v2012.05.0	
	
However you can see the history of the development without any rebase by following:

	<https://github.com/neub/wrs-sw-barebox/tree/wrs>	
	

Application
=============	

1. You first need to download the binary file: barebox-2012.05.0.tar.bz2
* Then you need to extract it to a folder, and go into it
* Initialize a git repo: `git init .`
* And finally you can apply the patches: git am 00*.patch
