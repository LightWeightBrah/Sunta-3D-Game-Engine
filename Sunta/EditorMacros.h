#pragma once

// If we use PROPERTY_FLOAT(intensity) this ##var will change to 'intensity'
// int x = (function(), 0)
// compiler see that it needs to assign some value to 'x' and it runs function()
// the ', 0' is required. This is how it works:

// (A, B)
//1. Do A  
//2. Ignore A
//3. Do B
//4. Return B as final output

//PROPERTY_FLOAT(intensity)
//int _inspector_intensity = (AddProperty("intensity", &intensity, Sunta::PropertyType::Float), 0)

#define PROPERTY_FLOAT(data) int _inspector_##data = (AddProperty(#data, &data, Sunta::PropertyType::Float), 0);