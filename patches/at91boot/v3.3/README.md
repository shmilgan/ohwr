Generation
============

These patches are generated using the following git:

	<https://github.com/neub/wrs-sw-at91bootstrap/tree/patcheable-master>
	
and by executing:

	git format-patch v3.3.0	
	
However you can see the history of the development without any rebase by following:

	<https://github.com/neub/wrs-sw-at91bootstrap/tree/master>	
	

Application
=============	

1. You first need to download the binary file: at91bootstrap-3-3.0.tar.gz
* Then you need to extract it to a folder, and go into it
* Initialize a git repo: `git init .`
* And finally you can apply the patches: git am 00*.patch
