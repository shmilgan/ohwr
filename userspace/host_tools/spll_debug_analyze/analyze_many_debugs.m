% Maciej Lipinski / CERN / 2014-10-22
% 
% scripts to analyzer debugging messages from the SoftPLL of the switch
% work in Matlab and Octave
% 
% HOW:
% 1. capture debugs using spll_dbg_read.c appliction
% 2. the above will write the debugs to 3 files (and output to sdout): mPLL.txt, bPLL.txt, hPLL.txt
% 3. best, put all 3 files to a folder, clean them up (i.e. remove header, and list line if not 
%    all columns written
% 4. provide folder name below
% 5. run and pray
% 

clear; 
close all;

history_offset = 10000;
future_offset = 2000;
option = 1;

[mpll, bpll, hpll, switchover] = plotSoftPLLdebugs('../2014-10-21',history_offset, future_offset, option);
mERR = mpll(:,3); bERR = bpll(:,3); hERR = hpll(:,3);
mY   = mpll(:,2); bY   = bpll(:,2); hY   = hpll(:,2);
anySO= mpll(:,6); % switchover

[mpll, bpll, hpll, switchover] = plotSoftPLLdebugs('../2014-10-21-1',history_offset, future_offset, option);
mERR = [mERR, mpll(:,3)]; bERR = [bERR, bpll(:,3)]; hERR = [hERR,hpll(:,3)];
mY   = [mY,   mpll(:,2)]; bY   = [bY,   bpll(:,2)]; hY   = [hY,  hpll(:,2)];

[mpll, bpll, hpll, switchover] = plotSoftPLLdebugs('../2014-10-22-1',history_offset, future_offset, option);
mERR = [mERR, mpll(:,3)]; bERR = [bERR, bpll(:,3)]; hERR = [hERR,hpll(:,3)];
mY   = [mY,   mpll(:,2)]; bY   = [bY,   bpll(:,2)]; hY   = [hY,  hpll(:,2)];

[mpll, bpll, hpll, switchover] = plotSoftPLLdebugs('../2014-10-22-2',history_offset, future_offset, option);
mERR = [mERR, mpll(:,3)]; bERR = [bERR, bpll(:,3)]; hERR = [hERR,hpll(:,3)];
mY   = [mY,   mpll(:,2)]; bY   = [bY,   bpll(:,2)]; hY   = [hY,  hpll(:,2)];

[mpll, bpll, hpll, switchover] = plotSoftPLLdebugs('../2014-10-22-3',history_offset, future_offset, option);
mERR = [mERR, mpll(:,3)]; bERR = [bERR, bpll(:,3)]; hERR = [hERR,hpll(:,3)];
mY   = [mY,   mpll(:,2)]; bY   = [bY,   bpll(:,2)]; hY   = [hY,  hpll(:,2)];


[mpll, bpll, hpll, switchover] = plotSoftPLLdebugs('../2014-10-22-4',history_offset, future_offset, option);
mERR = [mERR, mpll(:,3)]; bERR = [bERR, bpll(:,3)]; hERR = [hERR,hpll(:,3)];
mY   = [mY,   mpll(:,2)]; bY   = [bY,   bpll(:,2)]; hY   = [hY,  hpll(:,2)];


start = switchover - 1000;
finish= switchover + 1000;
draw(mERR, bERR, hERR, mY, anySO, switchover, start, finish)
start = switchover - 500;
finish= switchover + 100;
draw(mERR, bERR, hERR, mY, anySO, switchover, start, finish)
start = switchover - 1000;
finish= switchover - 1;
draw(mERR, bERR, hERR, mY, anySO, switchover, start, finish)


