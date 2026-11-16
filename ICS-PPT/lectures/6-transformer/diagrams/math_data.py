"""Vector helpers shared by the mathematical checks."""
import math

def matvec(matrix, vector):
    return [sum(a*b for a, b in zip(row, vector)) for row in matrix]

def softmax(values):
    largest = max(values)
    exponentials = [math.exp(value-largest) for value in values]
    total = sum(exponentials)
    return [value / total for value in exponentials]
