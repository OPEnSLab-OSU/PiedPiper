%Number_Time.wav/png

%Determine if /Spectrograms exists; create it if it doesn't.
%Determine if /ProcessedSpectrograms exists; create it if it doesn't
%Determine if /DetectionAudio exists; create it if it doesn't
%Open "DETS.txt"
%Loop:
%   Read detection audio chunk
%   Output WAVE file
%   Output raw spectrogram
%   Output processed spectrogram
clear
clc
clf

sampleFreq = 2048;
windowSize = 256;

detectionDataFileDir = 'lateSept\oak\';

mkdir([detectionDataFileDir, 'Detection Audio'])
mkdir([detectionDataFileDir, 'Spectrograms']);
mkdir([detectionDataFileDir, 'Processed Spectrograms']);

dets = fopen([detectionDataFileDir, 'DETS.txt']);

aud = zeros([2048*8,1]);

n = 0;
i = 1;

while (1)
    dat = fgetl(dets);
    
    if dat == -1
        aud = aud - mean(aud);
        aud2 = max(min(aud,200), -200);
        
        audiowrite([detectionDataFileDir, 'Detection Audio\', fname, '.wav'], aud2 ./ max(abs(aud2)), 2048);
        
        freqs = processSignal(aud, 2048, windowSize);
        clf
        freqs = freqs';
        freqs = min(freqs, 2000);
        fs = size(freqs);
        x = linspace(0,8,fs(2));
        y = linspace(1024,0,windowSize/2);
        %freqs = min(freqs, 10000);
        s = imagesc(x,y,flip(freqs));
        axis xy
        xlabel("Time (sec)");
        ylabel("Frequency (Hz)");
        saveas(s, [detectionDataFileDir, 'Spectrograms\', fname, '.png'])
        
        freqs = processSignalRect(aud, 2048, windowSize, 1);
        clf
        freqs = freqs';
        freqs = min(freqs, 2000);
        fs = size(freqs);
        x = linspace(0,8,fs(2));
        y = linspace(1024,0,windowSize/2);
        %freqs = min(freqs, 10000);
        s = imagesc(x,y,flip(freqs));
        axis xy
        xlabel("Time (sec)");
        ylabel("Frequency (Hz)");
        saveas(s, [detectionDataFileDir, 'Processed Spectrograms\', fname, '.png'])
        
        i = 1;
        aud = zeros([2048*8,1]);
        
        break
    end
    
    if contains(dat, 'NUMBER')
        %Close previous file if n != 0
        %Increment pointer to time designator
        %Get time of next detection
        %Increment num by 1
        %Create file
        if (n ~= 0)
            aud = aud - mean(aud);
            aud2 = max(min(aud,200), -200);
            
            audiowrite([detectionDataFileDir, 'Detection Audio\', fname, '.wav'], aud2 ./ max(abs(aud2)), 2048);
            
            freqs = processSignal(aud, 2048, windowSize);
            clf
            freqs = freqs';
            freqs = min(freqs, 2000);
            fs = size(freqs);
            x = linspace(0,8,fs(2));
            y = linspace(1024,0,windowSize/2);
            %freqs = min(freqs, 10000);
            s = imagesc(x,y,flip(freqs));
            axis xy
            xlabel("Time (sec)");
            ylabel("Frequency (Hz)");
            saveas(s, [detectionDataFileDir, 'Spectrograms\', fname, '.png'])
            
            freqs = processSignalRect(aud, 2048, windowSize, 1);
            clf
            freqs = freqs';
            freqs = min(freqs, 2000);
            fs = size(freqs);
            x = linspace(0,8,fs(2));
            y = linspace(1024,0,windowSize/2);
            %freqs = min(freqs, 10000);
            s = imagesc(x,y,flip(freqs));
            axis xy
            xlabel("Time (sec)");
            ylabel("Frequency (Hz)");
            saveas(s, [detectionDataFileDir, 'Processed Spectrograms\', fname, '.png'])
            
            i = 1;
            aud = zeros([2048*8,1]);
        end
        
        fgetl(dets);
        fgetl(dets);
        t = fgetl(dets);
        
        fgetl(dets);
        
        n = n + 1;
        
        fname = [int2str(n), '_', t];
    else
        aud(i) = str2double(dat);
        i = i + 1;
    end
    
end

%signal = signal - mean(signal);
%sound(signal / max(signal), 2048);
%plot(signal);

%signal = normalizeAudio(signal, windowSize);

%freqs = processSignal(signal, sampleFreq, windowSize);
%freqs = log(freqs);
%freqs = freqs .^ 2;
%freqs = freqs';
%freqs = min(freqs, 2000);
%fs = size(freqs);
%x = linspace(0,8,fs(2));
%y = linspace(0,1024,windowSize/2);
%freqs = min(freqs, 10000);
%s=surf(freqs);
%s=surf(x,y,freqs);
%s.EdgeColor = 'None';
%xlabel("Time (sec)");
%ylabel("Frequency (Hz)");

%plot(signal)

function [freqs] = processSignal(input, sFreq, winSize)
sampleCount = length(input);
windowCount = floor(sampleCount / winSize);

input = input(1:(windowCount*winSize));

freqs = zeros([winSize, windowCount]);
freqs(:) = input(:);
freqs = freqs';

for i = 1:windowCount
    freqs(i,:) = freqs(i,:) .* hamming(winSize)';
    freqs(i,:) = abs(fft(freqs(i,:)));
end

freqs = freqs(:,[1:(winSize/2)]);
end

function [freqs] = processSignalRect(input, sFreq, winSize, doSmooth)

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