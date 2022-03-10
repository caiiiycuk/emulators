"use strict";
exports.__esModule = true;
exports.webGl = void 0;
var vsSource = "\nattribute vec4 aVertexPosition;\nattribute vec2 aTextureCoord;\n\nvarying highp vec2 vTextureCoord;\n\nvoid main(void) {\n  gl_Position = aVertexPosition;\n  vTextureCoord = aTextureCoord;\n}\n";
var fsSource = "\nvarying highp vec2 vTextureCoord;\nuniform sampler2D uSampler;\n\n\nvoid main(void) {\n  highp vec4 color = texture2D(uSampler, vTextureCoord);\n  gl_FragColor = vec4(color.r, color.g, color.b, 1.0);\n}\n";
function webGl(layers, ci) {
    var canvas = layers.canvas;
    var gl = canvas.getContext("webgl");
    if (gl === null) {
        throw new Error("Unable to create webgl context on given canvas");
    }
    var shaderProgram = initShaderProgram(gl, vsSource, fsSource);
    var vertexPosition = gl.getAttribLocation(shaderProgram, "aVertexPosition");
    var textureCoord = gl.getAttribLocation(shaderProgram, "aTextureCoord");
    var uSampler = gl.getUniformLocation(shaderProgram, "uSampler");
    initBuffers(gl, vertexPosition, textureCoord);
    var texture = gl.createTexture();
    gl.bindTexture(gl.TEXTURE_2D, texture);
    gl.texParameteri(gl.TEXTURE_2D, gl.TEXTURE_WRAP_S, gl.CLAMP_TO_EDGE);
    gl.texParameteri(gl.TEXTURE_2D, gl.TEXTURE_WRAP_T, gl.CLAMP_TO_EDGE);
    gl.texParameteri(gl.TEXTURE_2D, gl.TEXTURE_MIN_FILTER, gl.LINEAR);
    gl.texParameteri(gl.TEXTURE_2D, gl.TEXTURE_MAG_FILTER, gl.LINEAR);
    var pixel = new Uint8Array([0, 0, 0]);
    gl.texImage2D(gl.TEXTURE_2D, 0, gl.RGB, 1, 1, 0, gl.RGB, gl.UNSIGNED_BYTE, pixel);
    gl.useProgram(shaderProgram);
    gl.activeTexture(gl.TEXTURE0);
    gl.uniform1i(uSampler, 0);
    var containerWidth = layers.width;
    var containerHeight = layers.height;
    var frameWidth = 0;
    var frameHeight = 0;
    var onResize = function () {
        var aspect = frameWidth / frameHeight;
        var width = containerWidth;
        var height = containerWidth / aspect;
        if (height > containerHeight) {
            height = containerHeight;
            width = containerHeight * aspect;
        }
        canvas.style.position = "relative";
        canvas.style.top = (containerHeight - height) / 2 + "px";
        canvas.style.left = (containerWidth - width) / 2 + "px";
        canvas.style.width = width + "px";
        canvas.style.height = height + "px";
    };
    var onResizeLayer = function (w, h) {
        containerWidth = w;
        containerHeight = h;
        onResize();
    };
    layers.addOnResize(onResizeLayer);
    var onResizeFrame = function (w, h) {
        frameWidth = w;
        frameHeight = h;
        canvas.width = frameWidth;
        canvas.height = frameHeight;
        gl.viewport(0, 0, frameWidth, frameHeight);
        onResize();
    };
    ci.events().onFrameSize(onResizeFrame);
    onResizeFrame(ci.width(), ci.height());
    var requestAnimationFrameId = null;
    var frame = null;
    var frameFormat = 0;
    ci.events().onFrame(function (rgb, rgba) {
        frame = rgb != null ? rgb : rgba;
        frameFormat = rgb != null ? gl.RGB : gl.RGBA;
        if (requestAnimationFrameId === null) {
            requestAnimationFrameId = requestAnimationFrame(updateTexture);
        }
    });
    var updateTexture = function () {
        gl.texImage2D(gl.TEXTURE_2D, 0, frameFormat, frameWidth, frameHeight, 0, frameFormat, gl.UNSIGNED_BYTE, frame);
        gl.drawArrays(gl.TRIANGLES, 0, 6);
        requestAnimationFrameId = null;
        frame = null;
    };
    ci.events().onExit(function () {
        layers.removeOnResize(onResizeLayer);
    });
}
exports.webGl = webGl;
function initShaderProgram(gl, vsSource, fsSource) {
    var vertexShader = loadShader(gl, gl.VERTEX_SHADER, vsSource);
    var fragmentShader = loadShader(gl, gl.FRAGMENT_SHADER, fsSource);
    var shaderProgram = gl.createProgram();
    gl.attachShader(shaderProgram, vertexShader);
    gl.attachShader(shaderProgram, fragmentShader);
    gl.linkProgram(shaderProgram);
    if (!gl.getProgramParameter(shaderProgram, gl.LINK_STATUS)) {
        throw new Error("Unable to initialize the shader program: " + gl.getProgramInfoLog(shaderProgram));
    }
    return shaderProgram;
}
function loadShader(gl, shaderType, source) {
    var shader = gl.createShader(shaderType);
    gl.shaderSource(shader, source);
    gl.compileShader(shader);
    if (!gl.getShaderParameter(shader, gl.COMPILE_STATUS)) {
        var info = gl.getShaderInfoLog(shader);
        gl.deleteShader(shader);
        throw new Error("An error occurred compiling the shaders: " + info);
    }
    return shader;
}
function initBuffers(gl, vertexPosition, textureCoord) {
    var positionBuffer = gl.createBuffer();
    gl.bindBuffer(gl.ARRAY_BUFFER, positionBuffer);
    var positions = [
        -1.0, -1.0, 0.0,
        1.0, -1.0, 0.0,
        1.0, 1.0, 0.0,
        -1.0, -1.0, 0.0,
        1.0, 1.0, 0.0,
        -1.0, 1.0, 0.0,
    ];
    gl.bufferData(gl.ARRAY_BUFFER, new Float32Array(positions), gl.STATIC_DRAW);
    gl.vertexAttribPointer(vertexPosition, 3, gl.FLOAT, false, 0, 0);
    gl.enableVertexAttribArray(vertexPosition);
    var textureCoordBuffer = gl.createBuffer();
    gl.bindBuffer(gl.ARRAY_BUFFER, textureCoordBuffer);
    var textureCoordinates = [
        0.0, 1.0,
        1.0, 1.0,
        1.0, 0.0,
        0.0, 1.0,
        1.0, 0.0,
        0.0, 0.0,
    ];
    gl.bufferData(gl.ARRAY_BUFFER, new Float32Array(textureCoordinates), gl.STATIC_DRAW);
    gl.vertexAttribPointer(textureCoord, 2, gl.FLOAT, false, 0, 0);
    gl.enableVertexAttribArray(textureCoord);
}
