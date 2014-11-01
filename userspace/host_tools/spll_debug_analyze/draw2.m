% Maciej Lipinski / CERN / 2014-10-22
% 
% scripts to analyzer debugging messages from the SoftPLL of the switch
% 
% options:
% 
% 
function draw2(mpll, bpll, hpll, switchover, start, finish, option)

unitScale = (1/((62.5-(62.5*((2^14)/(1+2^14))))*10^6)); % [s]
%  Xaxis     = (start:finish)*unitScale;
Xaxis     = (1:(finish-start+1))*unitScale;
to_ps     = 16000/(2^14); 

figure
subplot(4,1,1)
hold on
plot(Xaxis,mpll(start:finish,3)*to_ps,'b');
if(option == 3)
    plot(Xaxis,mpll(start:finish,5)*to_ps,'k');
end
plot(Xaxis,mpll(start:finish,7)*to_ps,'g');
plot(Xaxis,mpll(start:finish,8)*to_ps,'m');
plot(Xaxis,mpll(start:finish,6)*max(mpll(start:finish,3)),'r' );
if(option == 3)
   legend('corrected err (input to PI)','real error','long average','short average','switchover','Location','northwest');
else
   legend('err (input to PI)','long average','short average','switchover','Location','northwest');
end

title('mPLL');
xlabel('time [s]');
ylabel('phase [ps]');
axis tight
subplot(4,1,2)
hold on
plot(Xaxis,bpll(start:finish,3)*to_ps,'b');
plot(Xaxis,bpll(start:finish,7)*to_ps,'g');
plot(Xaxis,bpll(start:finish,8)*to_ps,'m');
plot(Xaxis,bpll(start:finish,5)*to_ps,'k');
plot(Xaxis,bpll(start:finish,6)*max(bpll(start:finish,3)),'r' );
legend('err','long average','short average','rememberd','switchover','Location','northwest');
title('bPLL');
xlabel('time [s]');
ylabel('phase [ps]');
axis tight
subplot(4,1,3)
plot(Xaxis,hpll(start:finish,3)*to_ps,'b',Xaxis,hpll(start:finish,6)*max(hpll(start:finish,3)),'r' );
legend('err','switchover','Location','northwest');
title('hPLL');
xlabel('time [s]');
ylabel('phase [ps]');
axis tight
subplot(4,1,4)
hold on
%  plot(Xaxis,mpll(start:finish,2),'b',switchover,max(mpll(start:finish,2)),'*r' );
plot(Xaxis,mpll(start:finish,2));
plot(Xaxis,mpll(start:finish,4),'g');
plot((switchover-start+1)*unitScale,max(mpll(start:finish,2)),'*r' );
legend('Y','long average','switchover','Location','northwest');
title('Y');
xlabel('time [s]');
ylabel('DAC out');
axis tight
return