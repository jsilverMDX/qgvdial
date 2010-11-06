////////////////////////////////////////////////////////////////////////////////
// All helper functions and their global variables sit in this file
////////////////////////////////////////////////////////////////////////////////
var compCbBox, cbBox;

////////////////////////////////////////////////////////////////////////////////
// Calculate the height/width of the current component
function calcFlowChildHeight () {
    if (parent.height > parent.width) {
        return (parent.height / 2);
    } else {
        return (parent.height);
    }
}
function calcFlowChildWidth () {
    if (parent.width > parent.height) {
        return ((parent.width - parent.spacing) / 2);
    } else {
        return (parent.width);
    }
}

////////////////////////////////////////////////////////////////////////////////
// Calculate the font point from the minimum of the height and width of the button
function btnFontPoint () {
    var m = (height < width? height : width);
    m = (m + 1) / 2;
    m = (m < 1 ? 1 : m);
    return (m);
}

////////////////////////////////////////////////////////////////////////////////
// Calculate the subtext font point from the main font point
function btnSubTextFontPoint () {
    var m = (btnFontPoint() + 2) / 3;
    m = (m == 0 ? 1 : m);
    return (m);
}
