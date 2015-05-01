#lang plai
(define-type animal
  [yacc (height number?)])

(define y (yacc 10))
(define x (yacc 15))

(test (yacc-height y) 10)
(test (yacc-height x) 15)


(define (f a)
  (type-case animal a
    [yacc (h)(* h 2.54)]))

(test (f y)(* 10 2.54))