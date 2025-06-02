% 参数初始化
g = 9.81; % 重力加速度 (m/s^2)
initial_velocity = 18; % 初始速度 (m/s)
initial_pitch_angle_1 = 30; % 第一个俯仰角（与水平面的夹角）(度)
initial_yaw_angle_1 = -7.3; % 第一个偏航角（水平面内与x轴的夹角）(度)
initial_pitch_angle_2 = 30; % 第二个俯仰角（与水平面的夹角）(度)
initial_yaw_angle_2 = -10; % 第二个偏航角（水平面内与x轴的夹角）(度)
time_of_flight = 5; % 飞行时间 (s)，这个时间可以根据需要调整
dt = 0.01; % 时间步长 (s)
time = 0:dt:time_of_flight; % 时间向量

% 将初始速度分解为x、y和z方向
initial_velocity_x_1 = initial_velocity * cosd(initial_pitch_angle_1) * cosd(initial_yaw_angle_1);
initial_velocity_y_1 = initial_velocity * cosd(initial_pitch_angle_1) * sind(initial_yaw_angle_1);
initial_velocity_z_1 = initial_velocity * sind(initial_pitch_angle_1);

initial_velocity_x_2 = initial_velocity * cosd(initial_pitch_angle_2) * cosd(initial_yaw_angle_2);
initial_velocity_y_2 = initial_velocity * cosd(initial_pitch_angle_2) * sind(initial_yaw_angle_2);
initial_velocity_z_2 = initial_velocity * sind(initial_pitch_angle_2);

% 初始化位置和速度向量
position_1 = zeros(3, length(time));
velocity_1 = zeros(3, length(time));
position_2 = zeros(3, length(time));
velocity_2 = zeros(3, length(time));
position_1(:, 1) = [0; 0; 0]; % 初始位置
velocity_1(:, 1) = [initial_velocity_x_1; initial_velocity_y_1; initial_velocity_z_1]; % 初始速度
position_2(:, 1) = [0; 0; 0]; % 初始位置
velocity_2(:, 1) = [initial_velocity_x_2; initial_velocity_y_2; initial_velocity_z_2]; % 初始速度

% 欧拉方法更新位置和速度
for i = 1:length(time)-1
    % 更新速度（在z方向上受到重力加速度影响）
    velocity_1(:, i+1) = velocity_1(:, i) + [0; 0; -g] * dt;
    velocity_2(:, i+1) = velocity_2(:, i) + [0; 0; -g] * dt;
    
    % 更新位置
    position_1(:, i+1) = position_1(:, i) + velocity_1(:, i) * dt;
    position_2(:, i+1) = position_2(:, i) + velocity_2(:, i) * dt;
    
    % 如果飞镖触地，则停止计算
    if position_1(3, i+1) < 0 && position_2(3, i+1) < 0 
        position_1(3, i+1) = 0;
        velocity_1(:, i+1:end) = repmat([0; 0; 0], 1, length(time) - i);
        position_1(:, i+1:end) = repmat(position_1(:, i+1), 1, length(time) - i);
        position_2(3, i+1) = 0;
        velocity_2(:, i+1:end) = repmat([0; 0; 0], 1, length(time) - i);
        position_2(:, i+1:end) = repmat(position_2(:, i+1), 1, length(time) - i);
        break;
    end
end

% 绘制轨迹
figure;
plot3(position_1(1, :), position_1(2, :), position_1(3, :), 'r'); % 红色轨迹
hold on; % 保持当前图形，使得后续绘图可以叠加在同一张图上
plot3(position_2(1, :), position_2(2, :), position_2(3, :), 'b'); % 蓝色轨迹
xlabel('X (m)');
ylabel('Y (m)');
zlabel('Z (m)');
title('3D Dart Trajectory');
grid on;

% 设置图形窗口的比例
ax = gca; % 获取当前坐标轴句柄
ax.DataAspectRatio = [1, 1, 1]; % 确保x、y、z轴的比例相同

% 调整视图，使原点在左侧
view(30, 30); % 方位角30度，仰角30度
