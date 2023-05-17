%====================================================================
% 【不采用】
%   参考Prasad评价方法，计算F值；------复现Arc论文数据分别为Traffic=0.8958、Prasad=0.4781、PCB=0.9447。
%   加载某数据集的检测结果——*.mat文件，
%   Ellipses_*.mat文件中存储的是此数据集中每张图片检测出的椭圆集(每个元素对应一张)；
%   利用每张图片的检测椭圆集和测试椭圆集的重叠比例，算出其F值；
%   最后利用每张图片的F值 (累加 / n) 求出平均值，即为最终数据集的F值。
%====================================================================
%====================================================================
% 两种评价方法：推荐使用A1-Lu的评价方法，对应Lu规格化参数后的数据集
% 	'A1'代表Lu-Arc评价方法：异TP累加法；分别按行和按列统计TP数量，将TP、FN和FP累加后计算F值。
% 	'B2'代表Jia-TIP评价方法；同TP平均法；统一按行统计TP数量，计算出每张图的F值后再求平均值。
%====================================================================


%% 统计某数据集对应的GT或Test的椭圆个数
% dataset = 'Dataset#2';
% files=dir(['./datasets/', dataset, '/gt/*.txt']); %需改
% path_GT=(['./datasets/', dataset, '/gt/']); %需改
% for f=1:length(files)
%     file_GT=[path_GT files(f).name];
%     data = importdata(file_GT); 
%     ellipse_param = reshape(data(2:end),5,data(1));
%     count(f) = data(1);
% end
% disp(sum(count));
%%====================================================================


% clear all; close all; clc;
function [resultFM, Precision, Recall] = ComputePerformance_all_Lu(selectEveFun)
    %%

%     选择被评价的数据集和相对路径，并加载对应某数据集的检测结果——*.mat文件 / 或者从新检测生成的det_*.txt文件中读取检测椭圆集的数据
     dataset = 'ged';  % Dataset#2、Dataset#1、Calibration
%     dataset = 'Prasad_plus';%Prasad Dataset，Prasad_plus
%     dataset = 'ged';%Prasad Dataset，Prasad_plus, new_traffic,new_ged
%     dataset = 'Industrial PCB Image Dataset';
%     dataset = 'Traffic Sign Dataset';
    files=dir(['./datasets/', dataset, '/gt0/*.txt']); %需改
    path_GT=(['./datasets/', dataset, '/gt0/']); %需改
%     % 从*.mat文件中加载检测椭圆集数据
%     load(['./evaluation/results_原/', dataset, '/Ellipses_CNC.mat']); % 针对不同的算法有不同的Ellipses_*.mat文件：Ellipses_lcs.mat、Ellipses_prasad.mat、Ellipses_rht.mat
%     algoParamTmp = Ellipses_CNC;
    % 从     
    filesDet=dir(['./datasets/', dataset, '/arcsl-oneStage-RDETR_KFLoss/50thEpoch/det/*.txt']);
    path_Det=(['./datasets/', dataset, '/arcsl-oneStage-RDETR_KFLoss/50thEpoch/det/']);
    
    % 标准圆和测试圆重叠比例的阈值
    %for bi = 0 : 6
    %beta=0.65+bi*0.05;  %0.8
    beta = 0.8;
    % 选择评价方法类型：selectFun = 'A1'代表A1：Lu-Arc评价方法，selectFun = 'B2'代表B2：Jia-TIP评价方法；
    if nargin < 1
        selectEveFun = 'A1';   % 'A1' / 'B2'
    end
    disp(selectEveFun);
    
    % 遍历某数据集GroundTruth的每个txt文件，即标准椭圆集
    for f=1:length(files)	% length(files)/5
        % file_GT代表当前gt_*.txt文件，即对应某图片的标准椭圆集，后续用来和测试椭圆集对比重叠程度
        file_GT=[path_GT files(f).name];
        %load(file_GT);

        % 加载当前gt文件导入标准椭圆集数据：代表某图片所含的椭圆个数+每个椭圆的五参数。并将每个椭圆五参数赋值到(5 X size)的矩阵ellipse_param中，即每列为一个椭圆
        %load(file_GT);
        data = importdata(file_GT); %loading the ground true datas. first data is the number of ellipses, while others are elliptic parameters.
        ellipse_param = reshape(data(2:end),5,data(1));%reshape to 5 x n, each column is 5 x 1,(cx,cy,a,b,theta_rad)
        %========================================================
        %show detected ellipses and gt ellipses.
        %path_img = 'D:\Graduate Design\Method comparing 2\Ellipse datasets\Industrial PCB Image Dataset\images\';
        %[path_string,name,ext] = fileparts(files(f).name);
        %im = imread([path_img, name, '.bmp']);
        %det_param = Param_det_cells{6, 1}{f};%detected ellipses
        %drawEllipses(det_param,im,'r');
        %drawEllipses(ellipse_param,im,'g');
        %========================================================

        % 将某图片的标准椭圆按cx升序排列，且仍是每列为一个椭圆。并将五参数的排列顺序改为(a,b,x,y,phi)
        ellipse_param=sortrows(ellipse_param.').';
        ellipse_param=ellipse_param([3 4 1 2 5],:); %适配，代码中是比较两个椭圆的格式是(a,b,x,y,phi)
        % // TODO：源代码格式为[3 4 2 1 5]且与上注释说明不一致，故改为[3 4 1 2 5]并经测验得知格式一致后可有略微提升；其实二者差异应该影响不大吧？
        
 %        det_param = algoParamTmp{f};
        % file_Det代表当前Det_*.txt文件，即对应某图片的测试椭圆集，后续用来和标准椭圆集对比重叠程度
        file_Det=[path_Det filesDet(f).name];
%         disp(file_Det);	% 输出当前评价的是哪个图片
        % 加载当前det文件导入测试椭圆集数据：代表某图片所含的椭圆个数+每个椭圆的五参数。并将每个椭圆五参数赋值到(5 X size)的矩阵det_param中，即每列为一个椭圆
        data2 = importdata(file_Det); %loading the ground true datas. first data is the number of ellipses, while others are elliptic parameters.
        det_param = reshape(data2(2:end),5,data2(1));%reshape to 5 x n, each column is 5 x 1,(cx,cy,a,b,theta_rad)
        % 若此图片的标准椭圆或测试椭圆集数量为0，则F值为0
        if size(det_param,2) == 0 || size(ellipse_param,2) == 0
            TP(f) = 0;
            FN(f) = size(ellipse_param,2)-TP(f);
            FP(f) = 0;
            % FMeasure(f) = 0;
            continue;
        end

        % 将测试椭圆按cx升序排列，且仍是每列为一个椭圆。并将五参数的排列顺序改为(a,b,x,y,phi)
        det_param=sortrows(det_param.').';
        det_param=det_param([3 4 1 2 5],:); %适配，代码中是比较两个椭圆的格式是(a,b,x,y,phi)
        % // TODO：源代码格式为[3 4 2 1 5]且与上注释说明不一致，故改为[3 4 1 2 5]并经测验得知格式一致后可有略微提升；其实二者差异应该影响不大吧？
        %     [gt,det]=meshgrid(1:size(ellipse_param,2),1:size(det_param,2));

        % 遍历标准椭圆集，并将当前标准圆与每个测试圆(遍历)比较，将比较结果存入Overlap矩阵对应位置---存入二者的重叠比例。
        Overlap = zeros(size(ellipse_param,2),size(det_param,2)); %存储M X N 的
        for ii=1:size(ellipse_param,2)
            for jj=1:size(det_param,2)
                max_x=max(ellipse_param(3,ii)+ellipse_param(1,ii),det_param(3,jj)+det_param(1,jj));
                max_y=max(ellipse_param(4,ii)+ellipse_param(1,ii),det_param(4,jj)+det_param(1,jj));
                Overlap(ii,jj)=check_overlap1(ellipse_param(:,ii),det_param(:,jj),[max_x,max_y]+5);
            end
        end
        
        % // TODO：这种行列分开统计的，好像思想上就是不对的？c：TP不一致，分别按行统计 / 按列统计，且计算时也分别使用TP
        % Precision(f)=sum(max(Overlap,[],1)>beta)/size(det_param,2); % 统计每列中最大重叠比例 > beta的比例数量，再与检测椭圆数量计算准确率
        % Recall(f)=sum(max(Overlap,[],2)>beta)/size(ellipse_param,2); % 统计每行中最大重叠比例 > beta的比例数量，再与标准椭圆数量计算召回率
        % if Precision(f) == 0 || Recall(f) == 0
        %     FMeasure(f) = 0;
        % else
        %     FMeasure(f) = 2*Precision(f)*Recall(f) / (Precision(f) + Recall(f));
        % end
        
        if selectEveFun == 'A1'
            % 根据此图片标准圆和测试圆的重叠比例，算出此图片对应的TP、FP和FN数量——a：TP不一致，分别按行统计 / 按列统计，但计算时全部使用按行统计的TP
            TP(f) = sum(max(Overlap,[],2)>beta);
            FN(f) = size(ellipse_param,2)-TP(f);
            FP(f) = size(det_param,2)-sum(max(Overlap,[],1)>beta);	% a和b差异之处
        end
        if selectEveFun  == 'B2'
            % 根据此图片标准圆和测试圆的重叠比例，算出此图片对应的TP、FP和FN数量——b：TP一致，全部按行统计，计算时自然也全部使用按行统计的TP
            TP(f) = sum(max(Overlap,[],2)>beta);
            FN(f) = size(ellipse_param,2)-TP(f);
            FP(f) = size(det_param,2)-TP(f);    % a和b差异之处
        end
        
        clear Overlap
    end

    if selectEveFun  == 'A1'
        % 【1、方式+ a：方式 = Lu评价方法】
        % 1、计算并输出某数据集的最终F值：将所有的TP、FP和FN累加后算出总的Precision、Recall，再2*Precision*Recall / (Precision + Recall) = 总的F值
        TPs = sum(TP, 2);
        FPs = sum(FP, 2);
        FNs = sum(FN, 2);
        if TPs == 0
            Precision = 0;
            Recall = 0;
            resultFM = 0;
        else
            Precision = TPs / (TPs + FPs);
            Recall = TPs / (TPs + FNs);
            resultFM = 2*Precision*Recall / (Precision + Recall);
        end
    end
    if selectEveFun  == 'B2'
        % 【2、方式 + b：方式 = Jia评价方法】
        % 2、计算并输出某数据集的最终F值：由每张图片的TP、FP和FN算出其F值，再将所有F值累加 / 图片个数 = 总的平均F值
        for f=1:length(files)
            if TP(f) == 0
                PrecisionArr(f) = 0;
                RecallArr(f) = 0;
                FMeasure(f) = 0;
            else
                PrecisionArr(f) = TP(f) / (TP(f) + FP(f));
                RecallArr(f) = TP(f) / (TP(f) + FN(f));
                FMeasure(f) = 2*PrecisionArr(f)*RecallArr(f) / (PrecisionArr(f) + RecallArr(f));
            end
        end
        Precision = sum(PrecisionArr, 2) ./ size(PrecisionArr, 2);
        Recall = sum(RecallArr, 2) ./ size(RecallArr, 2);
        resultFM = sum(FMeasure, 2) ./ size(FMeasure, 2);
    end
     disp([dataset, '_最终的评价值：[pre=', num2str(Precision), ']，[re=', num2str(Recall), ']，[f-m=', num2str(resultFM), ']']);
    
    %==================================================================
    % for iAlgo = 1:6
    %     TPs(iAlgo, bi+1) = sum(TP(iAlgo, :));
    %     FPs(iAlgo, bi+1) = sum(FP(iAlgo, :));
    %     FNs(iAlgo, bi+1) = sum(FN(iAlgo, :));
    %     if (TPs(iAlgo, bi+1)+FPs(iAlgo, bi+1)) == 0
    %         Precision(iAlgo, bi+1) = 1;
    %     else
    %         Precision(iAlgo, bi+1) =  TPs(iAlgo, bi+1)/( TPs(iAlgo, bi+1)+ FPs(iAlgo, bi+1));  %准确率
    %     end
    %     if ( TPs(iAlgo, bi+1)+ FNs(iAlgo, bi+1)) == 0
    %         Recall(iAlgo, bi+1) = 1;
    %     else
    %         Recall(iAlgo, bi+1)    = TPs(iAlgo, bi+1)/( TPs(iAlgo, bi+1)+ FNs(iAlgo, bi+1));  %召回率
    %     end
    %     FMeasure(iAlgo, bi+1)  = 2*Precision(iAlgo, bi+1)*Recall(iAlgo, bi+1)/(Precision(iAlgo, bi+1)+Recall(iAlgo, bi+1));
    % end
    %end

    % output_pathroot = ['.././', dataset, '/']; %需改
    % save(strcat(output_pathroot, 'TPs.mat'),'-mat', 'TPs'); %需改
    % save(strcat(output_pathroot, 'FPs.mat'),'-mat', 'FPs'); %需改
    % save(strcat(output_pathroot, 'FNs.mat'),'-mat', 'FNs'); %需改
    % save(strcat(output_pathroot, 'Precision.mat'),'-mat', 'Precision'); %需改
    % save(strcat(output_pathroot, 'Recall.mat'),'-mat', 'Recall'); %需改
    % save(strcat(output_pathroot, 'FMeasure.mat'),'-mat', 'FMeasure'); %需改
	%==================================================================
end
