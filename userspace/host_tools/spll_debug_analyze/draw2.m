% Maciej Lipinski / CERN / 2014-10-22
% 
% scripts to analyzer debugging messages from the SoftPLL of the switch
% 
function draw2(mpll, bpll, hpll, switchover, start, finish)


figure
subplot(4,1,1)
hold on
plot(start:finish,mpll(start:finish,3),'b',start:finish,mpll(start:finish,6)*max(mpll(start:finish,3)),'b' );
plot(start:finish,mpll(start:finish,7),'g');
plot(start:finish,mpll(start:finish,8),'m');
legend('err','long average','short average');
title('mPLL');
subplot(4,1,2)
hold on
plot(start:finish,bpll(start:finish,3),'b',start:finish,bpll(start:finish,6)*max(bpll(start:finish,3)),'b' );
plot(start:finish,bpll(start:finish,7),'g');
plot(start:finish,bpll(start:finish,8),'m');
legend('err','long average','short average');
title('bPLL');
subplot(4,1,3)
plot(start:finish,hpll(start:finish,3),'b',start:finish,hpll(start:finish,6)*max(hpll(start:finish,3)),'r' );
title('hPLL');
subplot(4,1,4)
hold on
plot(start:finish,mpll(start:finish,2),'b',switchover,max(mpll(start:finish,2)),'*r' );
plot(start:finish,mpll(start:finish,4),'g');
legend('Y','switchover','long average');
title('Y');
return