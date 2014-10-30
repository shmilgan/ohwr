function [mpll, bpll, hpll, switchover] = plotSoftPLLdebugs(path_name, history_offset, future_offset, option)

% options
% 0 : print nice plots
% >0: don't print, other optinos use this functions as a part of another computations

%  hack_offset = 510000;
close all;

if(history_offset <5000)
  disp('history_offset argument too small, make it min 5000')
  return
end

mpll_tmp=load('-ascii', sprintf('%s/mPLL.txt',path_name), 'data');
bpll_tmp=load('-ascii', sprintf('%s/bPLL.txt',path_name), 'data');
hpll_tmp=load('-ascii', sprintf('%s/hPLL.txt',path_name), 'data');

%  mpll_cleared = outliers(mpll_tmp(500000:end,:),[0.7, 0.7, 0, 0, 0, 0], 'mpll');
%  bpll_cleared = outliers(bpll_tmp(500000:end,:),[0.7, 0.7, 0, 0, 0, 0], 'bpll');
%  hpll_cleared = outliers(hpll_tmp(500000:end,:),[0.7, 0.7, 0, 0, 0, 0], 'hpll');

hack_offset = detectSwitchover(bpll_tmp,6) - 2*history_offset;

threshold_vec = zeros(size(mpll_tmp,2));
threshold_vec(1)=0.7;
threshold_vec(2)=0.7;
if(option == 2)
  threshold_vec(4)=0.3;
  threshold_vec(5)=0.7;
end
mpll_cleared = outliers(mpll_tmp(hack_offset:end,:),threshold_vec, 'mpll');
bpll_cleared = outliers(bpll_tmp(hack_offset:end,:),threshold_vec, 'bpll');
hpll_cleared = outliers(hpll_tmp(hack_offset:end,:),threshold_vec, 'hpll');


mpll_switchover = detectSwitchover(mpll_cleared,6)
bpll_switchover = detectSwitchover(bpll_cleared,6)
hpll_switchover = detectSwitchover(hpll_cleared,6)

%  switchover   = min([mpll_switchover,hpll_switchover,bpll_switchover])
switchover = history_offset;

mpll = [mpll_cleared(mpll_switchover-switchover+1:mpll_switchover-1,:);mpll_cleared(mpll_switchover:end,:)];
bpll = [bpll_cleared(bpll_switchover-switchover+1:bpll_switchover-1,:);bpll_cleared(bpll_switchover+1:end,:);zeros(size(mpll_cleared(mpll_switchover:end,:)))];
hpll = [hpll_cleared(hpll_switchover-switchover+1:hpll_switchover-1,:);hpll_cleared(hpll_switchover:end,:)];

tmp_len=length(mpll);
if(tmp_len - switchover < future_offset)
   mpll = [mpll(:,:);zeros(switchover+future_offset-tmp_len,size(mpll,2))];
elseif(tmp_len - switchover > future_offset)
   mpll = mpll(1:(switchover+future_offset),:);
end
size(mpll)
tmp_len = length(bpll);
if(tmp_len - switchover < future_offset)
   bpll = [bpll;zeros(switchover+future_offset-tmp_len,size(bpll,2))];
elseif(tmp_len - switchover > future_offset)
   bpll = bpll(1:(switchover+future_offset),:);
end
size(bpll)
tmp_len = length(hpll);
if(tmp_len - switchover < future_offset)
   hpll = [hpll;zeros(switchover+future_offset-tmp_len,size(hpll,2))];
elseif(tmp_len  - switchover > future_offset)
   hpll = hpll(1:(switchover+future_offset),:);
end
size(hpll)
mpll_switchover = detectSwitchover(mpll,6)
bpll_switchover = detectSwitchover(bpll,6)
hpll_switchover = detectSwitchover(hpll,6)

%  finish_plots=length(bpll);

if(option == 2)
  start = switchover - 1000;
  finish= switchover + 1000;
  draw2(mpll, bpll, hpll, switchover, start, finish);
  start = switchover - 500;
  finish= switchover + 100;
  draw2(mpll, bpll, hpll, switchover, start, finish);
  start = switchover - 1000;
  finish= switchover - 1;
  draw2(mpll, bpll, hpll, switchover, start, finish);
  return
end

if(option > 0)
  return
end


figure %1
hold on;
plot(mpll(:,3),'g')
plot(bpll(:,3),'b')
plot(hpll(:,3),'r')
%  
%  plot(mpll(start:finish,3),'g')
%  plot(bpll(start:finish,3),'b')
%  plot(hpll(start:finish,3),'r')

legend('mpll','bpll','hpll')


finish = min([length(mpll),length(hpll),length(bpll)]) -10

figure %2
subplot(4,1,1)
plot(1:finish,mpll(1:finish,3),'b',1:finish,mpll(1:finish,6)*max(mpll(1:finish,3)),'r' );
title('mPLL');
subplot(4,1,2)
plot(1:finish,bpll(1:finish,3),'b',1:finish,bpll(1:finish,6)*max(bpll(1:finish,3)),'r' );
title('bPLL');
subplot(4,1,3)
plot(1:finish,hpll(1:finish,3),'b',1:finish,hpll(1:finish,6)*max(hpll(1:finish,3)),'r' );
title('hPLL');
subplot(4,1,4)
plot(1:finish,mpll(1:finish,2),'b',switchover,max(mpll(1:finish,2)),'*r' );
title('Y');


figure %3

start  = switchover - ceil(0.1*switchover)
finish = switchover + ceil(0.1*switchover)

subplot(4,1,1)
plot(start:finish,mpll(start:finish,3),'b',start:finish,mpll(start:finish,6)*max(mpll(start:finish,3)),'r' );
title('mPLL');
subplot(4,1,2)
plot(start:finish,bpll(start:finish,3),'b',start:finish,bpll(start:finish,6)*max(bpll(start:finish,3)),'r' );
title('bPLL');
subplot(4,1,3)
plot(start:finish,hpll(start:finish,3),'b',start:finish,hpll(start:finish,6)*max(hpll(start:finish,3)),'r' );
title('hPLL');
subplot(4,1,4)
plot(start:finish,mpll(start:finish,2),'b',switchover,max(mpll(start:finish,2)),'*r' );
title('Y');


%  subplot(4,1,1)
%  plot(mpll(start:finish,3));
%  title('mPLL');
%  subplot(4,1,2)
%  plot(bpll(start:finish,3));
%  title('bPLL');
%  subplot(4,1,3)
%  plot(hpll(start:finish,3));
%  title('hPLL');
%  subplot(4,1,4)
%  plot(mpll(start:finish,2));
%  title('Y');
%  

figure %4

finish = switchover -1;

subplot(4,1,1)
plot(start:finish,mpll(start:finish,3),'b',start:finish,mpll(start:finish,6)*max(mpll(start:finish,3)),'r' );
title('mPLL');
subplot(4,1,2)
plot(start:finish,bpll(start:finish,3),'b',start:finish,bpll(start:finish,6)*max(bpll(start:finish,3)),'r' );
title('bPLL');
subplot(4,1,3)
plot(start:finish,hpll(start:finish,3),'b',start:finish,hpll(start:finish,6)*max(hpll(start:finish,3)),'r' );
title('hPLL');
subplot(4,1,4)
plot(start:finish,mpll(start:finish,2),'b',switchover,max(mpll(start:finish,2)),'*r' );
title('Y');

%  
%  subplot(4,1,1)
%  plot(mpll(start:finish,3));
%  title('mPLL');
%  subplot(4,1,2)
%  plot(bpll(start:finish,3));
%  title('bPLL');
%  subplot(4,1,3)
%  plot(hpll(start:finish,3));
%  title('hPLL');
%  subplot(4,1,4)
%  plot(mpll(start:finish,2));
%  title('Y');

figure %5

start = switchover - 1000;
finish= switchover + 1000;

subplot(4,1,1)
plot(start:finish,mpll(start:finish,3),'b',start:finish,mpll(start:finish,6)*max(mpll(start:finish,3)),'r' );
title('mPLL');
subplot(4,1,2)
plot(start:finish,bpll(start:finish,3),'b',start:finish,bpll(start:finish,6)*max(bpll(start:finish,3)),'r' );
% Maciej Lipinski / CERN / 2014-10-22
% 
% scripts to analyzer debugging messages from the SoftPLL of the switch
% 
title('bPLL');
subplot(4,1,3)
plot(start:finish,hpll(start:finish,3),'b',start:finish,hpll(start:finish,6)*max(hpll(start:finish,3)),'r' );
title('hPLL');
subplot(4,1,4)
plot(start:finish,mpll(start:finish,2),'b',switchover,max(mpll(start:finish,2)),'*r' );
title('Y');

%  
%  subplot(4,1,1)
%  plot(mpll(start:finish,3));
%  title('mPLL');
%  subplot(4,1,2)
%  plot(bpll(start:finish,3));
%  title('bPLL');
%  subplot(4,1,3)
%  plot(hpll(start:finish,3));
%  title('hPLL');
%  subplot(4,1,4)
%  plot(mpll(start:finish,2));
%  title('Y');


figure %6

start = switchover - 200;
finish= switchover + 200;

subplot(4,1,1)
plot(start:finish,mpll(start:finish,3),'b',start:finish,mpll(start:finish,6)*max(mpll(start:finish,3)),'r' );
title('mPLL');
subplot(4,1,2)
plot(start:finish,bpll(start:finish,3),'b',start:finish,bpll(start:finish,6)*max(bpll(start:finish,3)),'r' );
title('bPLL');
subplot(4,1,3)
plot(start:finish,hpll(start:finish,3),'b',start:finish,hpll(start:finish,6)*max(hpll(start:finish,3)),'r' );
title('hPLL');
subplot(4,1,4)
plot(start:finish,mpll(start:finish,2),'b',switchover,max(mpll(start:finish,2)),'*r' );
title('Y');

figure %7

start = switchover - 500;
finish= switchover - 20;

subplot(4,1,1)
plot(start:finish,mpll(start:finish,3),'b',start:finish,mpll(start:finish,6)*max(mpll(start:finish,3)),'r' );
title('mPLL');
subplot(4,1,2)
plot(start:finish,bpll(start:finish,3),'b',start:finish,bpll(start:finish,6)*max(bpll(start:finish,3)),'r' );
title('bPLL');
subplot(4,1,3)
plot(start:finish,hpll(start:finish,3),'b',start:finish,hpll(start:finish,6)*max(hpll(start:finish,3)),'r' );
title('hPLL');
subplot(4,1,4)
plot(start:finish,mpll(start:finish,2),'b',switchover,max(mpll(start:finish,2)),'*r' );
title('Y');
