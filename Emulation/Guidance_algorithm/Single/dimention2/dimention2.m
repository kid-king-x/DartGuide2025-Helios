% ��ʼ����
angle0 = 30  * pi / 180; %����Ƕ�
angle1 = 50 * pi / 180; %������ߺ�ı�ĽǶ�
v0 = 18; % ��ʼ�ٶ� (m/s)
g = 9.81; % �������ٶ� (m/s^2)
m = 0.21; % �������� (kg)
f_lift = 0.69; % ������N��
f_motor = 0.8; % ���� (N)
f_air = 0.41;% ��������(N)
f_weight = m * g; % ����(N)

dt = 0.01; % ʱ�䲽�� (s)

% ��ʼ������
t = 0; % ʱ�� (s)
x = 0; % ˮƽλ�� (m)
y = 0; % ��ֱλ�� (m)
vx = v0 * cos(angle0); % ˮƽ�����ٶ� (m/s)
vy = v0 * sin(angle0); % ��ֱ�����ٶ� (m/s)
theta = angle0; % ��ǰ������

% ���ڴ洢�켣�͸����ǵ�����
trajectory_x = [];
trajectory_y = [];
theta_values = [];

% ģ�����
while vy >= 0 
    % ������������
    Fy = -m * g + f_lift * cos(angle0) - (f_air + f_motor) * sin(angle0); % ��ֱ���������
    Fx = -f_lift * sin(angle0) - (f_air + f_motor) * cos(angle0);
    
    % �����ٶ�
    ax = Fx / m; % 
    ay = Fy / m; % ��ֱ������ٶ�
    vx = vx + ax * dt;
    vy = vy + ay * dt;
   
    
    % ����λ��
    x = x + vx * dt;
    y = y + vy * dt;
    
    % �洢�켣�͸�����
    trajectory_x = [trajectory_x, x];
    trajectory_y = [trajectory_y, y];
    theta_values = [theta_values, theta];
    
    % ����ʱ��
    t = t + dt;
end

while vy < 0 && y > 0
    ay =  -5;
    ax = 0;
    vx = vx + ax * dt;
    vy = vy + ay * dt;
    
    % ����λ��
    x = x + vx * dt;
    y = y + vy * dt;
    
    % �洢�켣�͸�����
    trajectory_x = [trajectory_x, x];
    trajectory_y = [trajectory_y, y];
    theta_values = [theta_values, theta];
    
    % ����ʱ��
    t = t + dt;
end


% ���ƹ켣ͼ
figure;
subplot(1, 2, 1);
plot(trajectory_x, trajectory_y, 'b');
xlabel('ˮƽλ�� (m)');
ylabel('��ֱλ�� (m)');
title('���彵��켣');
grid on;


