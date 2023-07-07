clear
clc
clf

%Recursively search directories to find "DATA" occurances

dataFiles = FindAndParseData(pwd, 5);

ParseData('C:\Users\admin\Desktop\2022 Pied Piper traps\Stag Hollow\SH Trap 4\DATA 09 07')

for k = 1:length(dataFiles)
    ParseData(dataFiles{k});
end

function ParseData(directory)
    files = dir(directory);
    
    parsedDataFolder = [directory, ' - PARSED'];
    
    if (exist(parsedDataFolder, 'file'))
        fprintf(['WARNING: Directory ', replace(parsedDataFolder, '\', '/'), 'already exists!'])
        return;
    else
        mkdir(parsedDataFolder)
        mkdir([parsedDataFolder, '\', 'Photos'])
        mkdir([parsedDataFolder, '\', 'Detections'])
        mkdir([parsedDataFolder, '\', 'Detections\', 'Spectrograms'])
        mkdir([parsedDataFolder, '\', 'Detections\', 'Processed Spectrograms'])
        mkdir([parsedDataFolder, '\', 'Detections\', 'Audio'])
    end
    
    photoFolderName = 'PHOTO';
    detsFolderName = 'DETS';
    
    for i = 1:length(files)
        if (contains(files(i).name, 'PHOTO'))
            photoFolderName = files(i).name;
    
        elseif (contains(files(i).name, 'DETS'))
            detsFolderName = files(i).name;
        end
    end
    
    photoMetaData = readmatrix([directory, '\', photoFolderName, '\PHOTO.txt']);
    
    for i = 1:max(photoMetaData(:,1))
        mkdir([parsedDataFolder, '\Photos\', sprintf('Detection %d', i)]);
    end
    
    mkdir([parsedDataFolder, '\Photos\Control'])
    photoMetaDataSize = size(photoMetaData);
    
    for i = 1:photoMetaDataSize(1)
        try
            if (photoMetaData(i,1) == 0)
                %copyfile([directory, sprintf('\PHOTO\%d.jpg', i)], [parsedDataFolder, '\Photos\Control']);
                copyfile([directory, '\', photoFolderName, sprintf('\\%d.jpg', i)], [parsedDataFolder, sprintf('\\Photos\\Control\\Control_%d-Y_%d-MO_%d-D_%d-H_%d-M_%d-S_%d.jpg', i, photoMetaData(i,2), photoMetaData(i,3), photoMetaData(i,4), photoMetaData(i,5), photoMetaData(i,6), photoMetaData(i,7))]);
            else
                copyfile([directory, '\', photoFolderName, sprintf('\\%d.jpg', i)], [parsedDataFolder, sprintf('\\Photos\\Detection %d\\Detection_%d-Number_%d-Y_%d-MO_%d-D_%d-H_%d-M_%d-S_%d.jpg', photoMetaData(i,1), photoMetaData(i,1), i, photoMetaData(i,2), photoMetaData(i,3), photoMetaData(i,4), photoMetaData(i,5), photoMetaData(i,6), photoMetaData(i,7))]);
            end
        catch
            continue
        end
    end
    
    %Find and open DETS.txt
    detectionMetaData = readmatrix([directory, '\', detsFolderName, '\DETS.TXT']);
    detectionMetaDataSize = size(detectionMetaData);
    
    %Generate and save audio and spectrograms for each detection
    for i = 1:detectionMetaDataSize(1)
        try
            aud = readmatrix([directory, '\', detsFolderName, sprintf('\\%d.txt', i)]);
            aud = aud - mean(aud);
            aud = aud ./ 2048;
        
            audiowrite([parsedDataFolder, sprintf('\\Detections\\Audio\\Detection_%d-Y_%d-MO_%d-D_%d-H_%d-M_%d-S_%d.wav', i, detectionMetaData(i,2), detectionMetaData(i,3), detectionMetaData(i,4), detectionMetaData(i,5), detectionMetaData(i,6), detectionMetaData(i,7))], aud, 4096);
        
            %Generate spectrograms
            unprocessedSpect = GenerateSpectrogram(aud, 4096, 256, 0);
            saveas(unprocessedSpect, [parsedDataFolder, sprintf('\\Detections\\Spectrograms\\Detection_%d-Y_%d-MO_%d-D_%d-H_%d-M_%d-S_%d.png', i, detectionMetaData(i,2), detectionMetaData(i,3), detectionMetaData(i,4), detectionMetaData(i,5), detectionMetaData(i,6), detectionMetaData(i,7))])
    
            processedSpect = GenerateSpectrogram(aud, 4096, 256, 1);
            saveas(processedSpect, [parsedDataFolder, sprintf('\\Detections\\Processed Spectrograms\\Detection_%d-Y_%d-MO_%d-D_%d-H_%d-M_%d-S_%d.png', i, detectionMetaData(i,2), detectionMetaData(i,3), detectionMetaData(i,4), detectionMetaData(i,5), detectionMetaData(i,6), detectionMetaData(i,7))])
        catch
            continue
        end
    end
end

function imgOut = GenerateSpectrogram(aud, sFreq, windowSize, doSmooth)
    freqs = ProcessSignal(aud, windowSize, doSmooth);
    clf
    freqs = freqs';
    freqs = min(freqs, 2000);
    fs = size(freqs);
    x = linspace(0,length(aud)/sFreq,fs(2));
    y = linspace(sFreq/2,0,windowSize/2);
    %freqs = min(freqs, 10000);
    imgOut = imagesc(x,y,flip(freqs));
    axis xy
    xlabel("Time (sec)");
    ylabel("Frequency (Hz)");
end

function [freqs] = ProcessSignal(input, winSize, doSmooth)
    sampleCount = length(input);
    windowCount = floor(sampleCount / winSize);
    
    input = input(1:(windowCount*winSize));
    
    freqs = zeros([winSize, windowCount]);
    freqs(:) = input(:);
    freqs = freqs';
    
    for i = 1:windowCount
        %freqs(i,:) = freqs(i,:) - mean(freqs(i,:));
        freqs(i,:) = freqs(i,:) .* hamming(winSize)';
        %freqs(i,:) = freqs(i,:) ./ max(max(freqs(i,:)));
        freqs(i,:) = abs(fft(freqs(i,:)));
    end
    
    freqs = freqs(:,[1:(winSize/2)]);
    %freqs(freqs < 0) = 0;
    
    %CORRECT ONE:
    if doSmooth
        
        for i = 1:winSize/2
            freqs(:,i) = smooth(freqs(:,i), 4);
        end
        
        for i = 1:windowCount
            freqs(i,:) = smooth(freqs(i,:), 4);
            freqs(i,:) = freqs(i,:) - smooth(freqs(i,:), 16);
        end
        
    end
    
    freqs(:,[1:6]) = 0;
    freqs(freqs < 0) = 0;
    %freqs = exp(freqs ./ 200);
end

function [output] = smooth(input, windowSize)
    pts = length(input);
    output = zeros(1, pts);
    
    for i=1:pts
        leftSide = max(1, i - windowSize / 2);
        rightSide = min(pts, i + windowSize / 2);
        ct = 0;
        
        for v = leftSide:rightSide
            output(i) = output(i) + input(v);
            ct = ct + 1;
        end
        
        output(i) = output(i) / ct;
        
    end
end

function files = FindAndParseData(directory, depth)

    arguments
        directory (1,:) char
        depth (1,1) double = 5
    end

    files = {};

    %fprintf([replace(['Descending ', directory], '\', '/'), '\n'])

    dirFiles = dir(directory);

    for i = 1:length(dirFiles)
        if (strcmp(dirFiles(i).name, '.') || strcmp(dirFiles(i).name, '..'))
            continue
        end

        if dirFiles(i).isdir
            possibleValidDataFiles = dir([directory, '\', dirFiles(i).name]);
            hasPhoto = false;
            hasDets = false;

            for p = 1:length(possibleValidDataFiles)
                if contains(possibleValidDataFiles(p).name, 'PHOTO') && possibleValidDataFiles(p).isdir
                    hasPhoto = true;
                end
            end

            for p = 1:length(possibleValidDataFiles)
                if contains(possibleValidDataFiles(p).name, 'DETS') && possibleValidDataFiles(p).isdir
                    hasDets = true;
                end
            end

            if (hasDets && hasPhoto)
                fprintf(['\n', replace(['Found valid file at ', directory, '\', dirFiles(i).name], '\', '/'), '\n\n'])
                files = [files, [directory, '\', dirFiles(i).name]];

            else
                if (depth > 0)
                    files = [files, FindAndParseData([directory, '\', dirFiles(i).name], depth - 1)];
                end
            end

        else
            if (depth > 0)
                files = [files, FindAndParseData([directory, '\', dirFiles(i).name], depth - 1)];
            end
        end
    end
end