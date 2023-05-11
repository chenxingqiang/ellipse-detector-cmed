% 返回某标准椭圆和某测试椭圆的重叠比例
function overlap_ratio=check_overlap1(ellipse_param1,ellipse_param2, size_im)

% 建立xy坐标系并根据图片size在平面上画网格
[pixels_x,pixels_y]=meshgrid(1:size_im(1),1:size_im(2));

% 读取标准椭圆集的五个参数，并计算其椭圆内像素点数
a1=ellipse_param1(1);
b1=ellipse_param1(2);
x1=ellipse_param1(3);
y1=ellipse_param1(4);
theta1=ellipse_param1(5);
f1=((pixels_x-x1)*sin(theta1)-(pixels_y-y1)*cos(theta1)).^2/b1^2+((pixels_x-x1)*cos(theta1)+(pixels_y-y1)*sin(theta1)).^2/a1^2-1;
pixels_inside_ellipse1=~(f1>0);

% 读取测试椭圆集的五个参数，并计算其椭圆内像素点数
a2=ellipse_param2(1);
b2=ellipse_param2(2);
x2=ellipse_param2(3);
y2=ellipse_param2(4);
theta2=ellipse_param2(5);
f2=((pixels_x-x2)*sin(theta2)-(pixels_y-y2)*cos(theta2)).^2/b2^2+((pixels_x-x2)*cos(theta2)+(pixels_y-y2)*sin(theta2)).^2/a2^2-1;
pixels_inside_ellipse2=~(f2>0);

% 根据二者像素点，计算出二者重叠比例并返回
overlap_ratio=1-sum(sum((xor(pixels_inside_ellipse1,pixels_inside_ellipse2))))/sum(sum((or(pixels_inside_ellipse1,pixels_inside_ellipse2))));

