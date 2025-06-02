% 初始参数
angle0 = 30  * pi / 180; %发射角度
angle1 = 50 * pi / 180; %到达最高后改变的角度
v0 = 18; % 初始速度 (m/s)
g = 9.81; % 重力加速度 (m/s^2)
m = 0.21; % 镖体质量 (kg)
f_lift = 0.69; % 升力（N）
f_motor = 0.8; % 推力 (N)
f_air = 0.41;% 空气阻力(N)
f_weight = m * g; % 重力(N)

dt = 0.01; % 时间步长 (s)

% 初始化变量
t = 0; % 时间 (s)
x = 0; % 水平位置 (m)
y = 0; % 垂直位置 (m)
vx = v0 * cos(angle0); % 水平方向速度 (m/s)
vy = v0 * sin(angle0); % 垂直方向速度 (m/s)
theta = angle0; % 当前俯仰角

% 用于存储轨迹和俯仰角的数组
trajectory_x = [];
trajectory_y = [];
theta_values = [];

% 模拟过程
while vy >= 0 
    % 计算重力分量
    Fy = -m * g + f_lift * cos(angle0) - (f_air + f_motor) * sin(angle0); % 垂直方向的总力
    Fx = -f_lift * sin(angle0) - (f_air + f_motor) * cos(angle0);
    
    % 更新速度
    ax = Fx / m; % 
    ay = Fy / m; % 垂直方向加速度
    vx = vx + ax * dt;
    vy = vy + ay * dt;
   
    
    % 更新位置
    x = x + vx * dt;
    y = y + vy * dt;
    
    % 存储轨迹和俯仰角
    trajectory_x = [trajectory_x, x];
    trajectory_y = [trajectory_y, y];
    theta_values = [theta_values, theta];
    
    % 更新时间
    t = t + dt;
end

while vy < 0 && y > 0
    ay =  -5;
    ax = 0;
    vx = vx + ax * dt;
    vy = vy + ay * dt;
    
    % 更新位置
    x = x + vx * dt;
    y = y + vy * dt;
    
    % 存储轨迹和俯仰角
    trajectory_x = [trajectory_x, x];
    trajectory_y = [trajectory_y, y];
    theta_values = [theta_values, theta];
    
    % 更新时间
    t = t + dt;
end


% 绘制轨迹图
figure;
subplot(1, 2, 1);
plot(trajectory_x, trajectory_y, 'b');
xlabel('水平位置 (m)');
ylabel('垂直位置 (m)');
title('镖体降落轨迹');
grid on;


