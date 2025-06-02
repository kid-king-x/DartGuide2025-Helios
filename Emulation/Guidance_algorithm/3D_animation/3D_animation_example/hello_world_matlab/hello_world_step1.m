% 创建一个世界对象。
world = sim3d.World();
% 创建一个角色对象。sim3d角色对象创建一个没有任何视觉网格的空角色。
actor = sim3d.Actor();
% 将角色加入世界
add(world,actor);
% 运行模拟集10秒，采样时间为0.02秒。
sampletime = 0.02;
stoptime = 10;
run(world,sampletime,stoptime);
delete(world);

